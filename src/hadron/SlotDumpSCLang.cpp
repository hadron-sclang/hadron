#include "hadron/SlotDumpSCLang.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/String.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"

namespace hadron {

// static
void SlotDumpSCLang::dump(ThreadContext* context, Slot slot, std::ostream& out) {
    out << "(\n"
        << "  var fail = { Error.new(\"lookup error!\").throw; };\n"
        << "  var references = IdentityDictionary.new;\n";

    std::unordered_set<Hash> encodedObjects;
    out << dumpValue(context, slot, out, encodedObjects)
        << ")\n";
}

// dumpValue should return a string that either defines a literal directly or defines a references.at({}) call that
// returns a created object. In the latter case dumpValue will have also defined that object in its own call.
// static
std::string SlotDumpSCLang::dumpValue(ThreadContext* context, Slot slot, std::ostream& out,
        std::unordered_set<Hash>& encodedObjects) {

    switch (slot.getType()) {
    case TypeFlags::kFloatFlag: {
        auto floatValue = slot.getFloat();
        if (std::isnan(floatValue)) { return "sqrt(-1.0)"; }
        if (std::isinf(floatValue)) { return floatValue < 0.0 ? "-inf" : "inf"; }
        return fmt::format("{}", floatValue);
    }

    case TypeFlags::kIntegerFlag:
        return fmt::format("{}", slot.getInt32());

    case TypeFlags::kBooleanFlag:
        return slot.getBool() ? "true" : "false";

    case TypeFlags::kNilFlag:
        return "nil";

    case TypeFlags::kObjectFlag:
        return dumpObject(context, library::ObjectBase::wrapUnsafe(slot), out, encodedObjects);

    case TypeFlags::kSymbolFlag: {
        auto contents = context->symbolTable->getString(slot.getSymbolHash()).view();
        std::string output = "'";
        for (size_t i = 0; i < contents.size(); ++i) {
            if (contents[i] == '\\' || contents[i] == '\'') {
                output += "\\";
            }
            output += contents[i];
        }
        output += "'";
        return output;
    }

    case TypeFlags::kCharFlag:
        return fmt::format("${}", slot.getChar());

    default:
        assert(false);
        return "";
    }
}

std::string SlotDumpSCLang::dumpObject(ThreadContext* context, library::ObjectBase object, std::ostream& out,
        std::unordered_set<Hash>& encodedObjects) {
    auto hash = static_cast<int32_t>(object.slot().identityHash());
    auto value = fmt::format("references.atFail({}, fail)", hash);
    if (encodedObjects.count(hash)) { return value; }

    encodedObjects.insert(hash);
    switch(object.className()) {
    case library::Array::nameHash(): {
        std::string def = fmt::format("  references.put({}, [\n", hash);
        library::Array array(object.slot());
        for (int32_t i = 0; i < array.size(); ++i) {
            def += "    " + dumpValue(context, array.at(i), out, encodedObjects) + ",\n";
        }
        out << def << "  ]);\n";
    } break;

    case library::IdentityDictionary::nameHash(): {
        std::string def = fmt::format("  references.put({}, IdentityDictionary.newFrom([\n", hash);
        library::IdentityDictionary dict(object.slot());
        Slot key = dict.nextKey(Slot::makeNil());
        while (key) {
            def += "    " + dumpValue(context, key, out, encodedObjects) + ", " +
                    dumpValue(context, dict.get(key), out, encodedObjects) + ",\n";
            key = dict.nextKey(key);
        }
        out << def << "  ]));\n";
    } break;

    case library::IdentitySet::nameHash(): {
        std::string def = fmt::format("  references.put({}, IdentitySet.newFrom([\n", hash);
        library::IdentitySet set(object.slot());
        Slot item = set.next(Slot::makeNil());
        while (item) {
            def += "    " + dumpValue(context, item, out, encodedObjects) + ",\n";
            item = set.next(item);
        }
        out << def << "  ]));\n";
    } break;

    case library::Int8Array::nameHash(): {
        out << fmt::format("  references.put({}, Int8Array[", hash);
        library::Int8Array array(object.slot());
        for (int32_t i = 0; i < array.size(); ++i) {
            out << fmt::format("{}, ", array.at(i));
        }
        out << "]);\n";
    } break;

    case library::SymbolArray::nameHash(): {
        std::string def = fmt::format("  references.put({}, SymbolArray[\n", hash);
        library::SymbolArray array(object.slot());
        for (int32_t i = 0; i < array.size(); ++i) {
            def += "    " + dumpValue(context, array.at(i).slot(), out, encodedObjects) + ", ";
        }
        out << def << "]);\n";
    } break;

    case library::String::nameHash(): {
        auto contents = library::String(object.slot()).view();
        std::string def = fmt::format("  references.put({}, \"", hash);
        for (size_t i = 0; i < contents.size(); ++i) {
            if (contents[i] == '\\' || contents[i] == '"') {
                def += "\\";
            }
            def += contents[i];
        }
        out << def << "\");\n";
    } break;

    default: {
        // Extract class name.
        library::Symbol className(context, Slot::makeSymbol(object.className()));
        std::string def = fmt::format("  references.put({}, {}.newCopyArgs(\n", hash, className.view(context));
        // Wrap array around object for sequential access to members.
        auto objectAsArray = library::Array::wrapUnsafe(object.slot());
        for (int32_t i = 0; i < objectAsArray.size(); ++i) {
            def += "    " + dumpValue(context, objectAsArray.at(i), out, encodedObjects) + ",\n";
        }
        out << def << "  ));\n";
    } break;
    }

    return value;
}

} // namespace hadron