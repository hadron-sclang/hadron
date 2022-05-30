#include "hadron/SlotDumpJSON.hpp"

#include "hadron/library/Symbol.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace hadron {

class SlotDumpJSON::Impl {
public:
    void dump(ThreadContext* context, Slot slot) {
        encodeSlot(context, slot, m_doc);

        rapidjson::Writer<rapidjson::StringBuffer> writer(m_buffer);
        m_doc.Accept(writer);
    }

    std::string_view json() const { return std::string_view(m_buffer.GetString(), m_buffer.GetSize()); }

private:
    rapidjson::Document m_doc;
    rapidjson::StringBuffer m_buffer;

    void encodeSlot(ThreadContext* context, Slot slot, rapidjson::Value& value) {
        switch (slot.getType()) {
        case TypeFlags::kFloatFlag:
            value = slot.getFloat();
            break;

        case TypeFlags::kIntegerFlag:
            value = slot.getInt32();
            break;

        case TypeFlags::kBooleanFlag:
            value = slot.getBool();
            break;

        case TypeFlags::kNilFlag:
            value = rapidjson::Value();
            break;

        case TypeFlags::kObjectFlag: {
        } break;

        // Encode symbols as strings directly in the JSON.
        case TypeFlags::kSymbolFlag: {
            auto symbol = library::Symbol(context, slot);
            value.SetString(symbol.view(context).data(), m_doc.GetAllocator());
        } break;

        // encode as an object of type "Char"
        case TypeFlags::kCharFlag: {
            value.SetObject();
            value.AddMember("_className", rapidjson::Value("Char"), m_doc.GetAllocator());
            rapidjson::Value charValue;
            charValue.SetString(fmt::format("{}", slot.getChar(), m_doc.GetAllocator()));
            value.AddMember("value", charValue, m_doc.GetAllocator());
        } break;

        // TODO
        case TypeFlags::kRawPointerFlag:
            assert(false); // TODO
            value = rapidjson::Value();
            break;

        // Slots should always have a single type flag.
        case TypeFlags::kNoFlags:
        case TypeFlags::kAllFlags:
            assert(false);
            value = rapidjson::Value();
            break;
        }
    }

};

SlotDumpJSON::SlotDumpJSON(): m_impl(std::make_unique<SlotDumpJSON::Impl>()) {}

void SlotDumpJSON::dump(ThreadContext* context, Slot slot) { m_impl->dump(context, slot); }

std::string_view SlotDumpJSON::json() const { return m_impl->json(); }

} // namespace hadron