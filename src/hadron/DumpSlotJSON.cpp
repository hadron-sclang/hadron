#include "hadron/DumpSlotJSON.hpp"

#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace {

void encodeSlot(ThreadContext* context, Slot slot, rapidjson::Document& doc) {

}

}

namespace hadron {

std::string DumpSlotJSON(ThreadContext* context, Slot slot) {
    rapidjson::Document document;

    encodeSlot(context, slot, doc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    // TODO: unfortunate string copy, can we repair?
    return std::string(buffer.GetString(), buffer.GetSize());
}

} // namespace hadron