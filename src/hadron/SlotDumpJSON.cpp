#include "hadron/SlotDumpJSON.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "spdlog/spdlog.h"

#include <unordered_set>

namespace hadron {

class SlotDumpJSON::Impl {
public:
    ~Impl() = default;

    void dump(ThreadContext* context, Slot slot, bool prettyPrint) {
        encodeSlot(context, slot, m_doc);

        bool result = false;
        if (prettyPrint) {
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(m_buffer);
            result = m_doc.Accept(writer);
        } else {
            rapidjson::Writer<rapidjson::StringBuffer> writer(m_buffer);
            result = m_doc.Accept(writer);
        }
        assert(result);
    }

    std::string_view json() const { return std::string_view(m_buffer.GetString(), m_buffer.GetSize()); }

private:
    rapidjson::Document m_doc;
    rapidjson::StringBuffer m_buffer;
    // To avoid cycles in objects we encode identity hashes with each object and store that hash in this set. On
    // repeated encounters to that object we just repeat the hash.
    std::unordered_set<Hash> m_encodedObjects;

    void encodeSlot(ThreadContext* context, Slot slot, rapidjson::Value& value) {
        auto& alloc = m_doc.GetAllocator();

        switch (slot.getType()) {
        case TypeFlags::kFloatFlag: {
            // rapidjson silently fails to encode NaN and inf doubles.
            auto dub = slot.getFloat();
            if (std::isnan(dub)) {
                value.SetObject();
                value.AddMember("_className", rapidjson::Value("Float"), alloc);
                value.AddMember("value", rapidjson::Value("nan"), alloc);
                return;
            }
            if (std::isinf(dub)) {
                value.SetObject();
                value.AddMember("_className", rapidjson::Value("Float"), alloc);
                value.AddMember("value", rapidjson::Value("inf"), alloc);
                return;
            }

            value = dub;
        }
            return;

        case TypeFlags::kIntegerFlag:
            value = slot.getInt32();
            return;

        case TypeFlags::kBooleanFlag:
            value = slot.getBool();
            return;

        case TypeFlags::kNilFlag:
            value = rapidjson::Value();
            return;

        // We dump most objects as dictionaries of their members, with the exception of some data structures like
        // Arrays, Sets, Dictionaries, etc, which need special treatment.
        case TypeFlags::kObjectFlag:
            encodeObject(context, slot, value);
            return;

        // Encode symbols as strings directly in the JSON. Strings are encoded as Objects.
        case TypeFlags::kSymbolFlag: {
            auto symbol = library::Symbol(context, slot);
            value.SetString(symbol.view(context).data(), alloc);
        }
            return;

        // encode as an object of type "Char"
        case TypeFlags::kCharFlag: {
            value.SetObject();
            value.AddMember("_className", rapidjson::Value("Char"), alloc);
            rapidjson::Value charValue;
            charValue.SetString(fmt::format("{}", slot.getChar()).data(), alloc);
            value.AddMember("value", charValue, alloc);
        }
            return;

        // TODO
        case TypeFlags::kRawPointerFlag:
            assert(false); // TODO
            value = rapidjson::Value();
            return;

        // Slots should always have a single type flag.
        case TypeFlags::kNoFlags:
        case TypeFlags::kAllFlags:
            assert(false);
            value = rapidjson::Value();
            return;
        }
    }

    void encodeObject(ThreadContext* context, Slot slot, rapidjson::Value& value) {
        auto& alloc = m_doc.GetAllocator();
        value.SetObject();

        // Either reference existing hash of already encoded object, or register hash of new object.
        auto identityHash = slot.identityHash();
        if (m_encodedObjects.count(identityHash)) {
            value.AddMember("_reference", rapidjson::Value(identityHash), alloc);
            return;
        }

        m_encodedObjects.emplace(identityHash);
        value.AddMember("_identityHash", rapidjson::Value(identityHash), alloc);

        auto slotObject = library::ObjectBase::wrapUnsafe(slot);

        // Extract class name.
        library::Symbol className(context, Slot::makeSymbol(slotObject.className()));

        rapidjson::Value classNameJSON;
        classNameJSON.SetString(className.view(context).data(), alloc);
        value.AddMember("_className", classNameJSON, alloc);

        // Look up Class object in class library.
        auto classDef = context->classLibrary->findClassNamed(className);
        if (!classDef) {
            SPDLOG_ERROR("failed to look up class '{}', name hash 0x{:08x} in class library", className.view(context),
                         className.hash());
            assert(false);
            return;
        }

        if (slotObject.className() == library::Array::nameHash()) {
            rapidjson::Value arrayElements;
            arrayElements.SetArray();
            auto array = library::Array(slot);
            for (int32_t i = 0; i < array.size(); ++i) {
                rapidjson::Value element;
                encodeSlot(context, array.at(i), element);
                arrayElements.PushBack(element, alloc);
            }
            value.AddMember("_elements", arrayElements, alloc);
            return;
        }

        if (slotObject.className() == library::IdentityDictionary::nameHash()) {
            rapidjson::Value elements;
            elements.SetArray();
            auto identityDict = library::IdentityDictionary(slot);
            Slot key = identityDict.nextKey(Slot::makeNil());
            while (key) {
                Slot keyValue = identityDict.get(key);
                assert(keyValue);
                rapidjson::Value keyValPair;
                keyValPair.SetObject();
                rapidjson::Value keyJson;
                encodeSlot(context, key, keyJson);
                keyValPair.AddMember("_key", keyJson, alloc);
                rapidjson::Value valueJson;
                encodeSlot(context, keyValue, valueJson);
                keyValPair.AddMember("_value", valueJson, alloc);
                elements.PushBack(keyValPair, alloc);
                key = identityDict.nextKey(key);
            }
            value.AddMember("_elements", elements, alloc);
            return;
        }

        if (slotObject.className() == library::IdentitySet::nameHash()) {
            rapidjson::Value setElements;
            setElements.SetArray();
            auto identitySet = library::IdentitySet(slot);
            Slot item = identitySet.next(Slot::makeNil());
            while (item) {
                rapidjson::Value element;
                encodeSlot(context, item, element);
                setElements.PushBack(element, alloc);
                item = identitySet.next(item);
            }
            value.AddMember("_elements", setElements, alloc);
            return;
        }

        if (slotObject.className() == library::Int8Array::nameHash()) {
            rapidjson::Value arrayElements;
            arrayElements.SetArray();
            auto array = library::Int8Array(slot);
            for (int32_t i = 0; i < array.size(); ++i) {
                arrayElements.PushBack(array.at(i), alloc);
            }
            value.AddMember("_elements", arrayElements, alloc);
            return;
        }

        if (slotObject.className() == library::SymbolArray::nameHash()) {
            rapidjson::Value arrayElements;
            arrayElements.SetArray();
            auto array = library::SymbolArray(slot);
            for (int32_t i = 0; i < array.size(); ++i) {
                auto symbol = array.at(i);
                rapidjson::Value symbolValue;
                symbolValue.SetString(symbol.view(context).data(), alloc);
                arrayElements.PushBack(symbolValue, alloc);
            }
            value.AddMember("_elements", arrayElements, alloc);
            return;
        }

        if (slotObject.className() == library::String::nameHash()) {
            rapidjson::Value elements;
            auto string = library::String(slot);
            elements.SetString(string.view().data(), alloc);
            value.AddMember("_elements", elements, alloc);
            return;
        }

        // We wrap non-collection objects in an Array for index-based access to their members.
        auto slotArray = library::Array::wrapUnsafe(slot);

        auto instVarNames = classDef.instVarNames();
        for (int32_t i = 0; i < instVarNames.size(); ++i) {
            auto varName = instVarNames.at(i);
            rapidjson::Value varNameJSON;
            varNameJSON.SetString(varName.view(context).data(), alloc);

            rapidjson::Value instVarValue;
            encodeSlot(context, slotArray.at(i), instVarValue);

            value.AddMember(varNameJSON, instVarValue, alloc);
        }
    }
};

SlotDumpJSON::SlotDumpJSON(): m_impl(std::make_unique<SlotDumpJSON::Impl>()) { }

SlotDumpJSON::~SlotDumpJSON() { }

void SlotDumpJSON::dump(ThreadContext* context, Slot slot, bool prettyPrint) {
    m_impl->dump(context, slot, prettyPrint);
}

std::string_view SlotDumpJSON::json() const { return m_impl->json(); }

} // namespace hadron