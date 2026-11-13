/*══════════════════════════════════════════════════════════════════════════════
 FILE: protobuf_inspector.cpp - TOOL: PROTOBUF INSPECTOR  (offline, no connection)

 PURPOSE:
   An interactive explorer for the MT5 gRPC protobuf contract. Browse messages,
   fields and enums, search by name, find which message declares a field, and list
   enum values — entirely from the descriptors compiled into the library. No MT5
   terminal, no network, no credentials.

 HOW IT WORKS:
   C++ protobuf keeps runtime descriptors. This tool reads the global generated
   descriptor pool (google::protobuf::DescriptorPool::generated_pool()), walking the
   mt5_term_api proto files linked into libmetarpc_cppmt5. (The CMake target links the
   library with --whole-archive so every generated file registers itself.)

 🚀 HOW TO RUN:  ./build/protobuf_inspector
══════════════════════════════════════════════════════════════════════════════*/

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>

using google::protobuf::DescriptorPool;
using google::protobuf::FileDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::OneofDescriptor;

namespace {

// The mt5_term_api proto files (from proto/mt5_term_api.desc). The generated code registers
// each of these in the generated pool at static-init time.
const std::vector<std::string> kProtoFiles = {
    "mt5-term-api-account-helper.proto",   "mt5-term-api-account-information.proto",
    "mt5-term-api-accounts.proto",         "mt5-term-api-admin.proto",
    "mt5-term-api-charts.proto",           "mt5-term-api-connection.proto",
    "mt5-term-api-demo-account.proto",     "mt5-term-api-gui.proto",
    "mt5-term-api-health-check.proto",     "mt5-term-api-internal-charts.proto",
    "mt5-term-api-market-info.proto",      "mt5-term-api-public-account.proto",
    "mt5-term-api-quote-history.proto",    "mt5-term-api-server-timezone.proto",
    "mt5-term-api-subscriptions.proto",    "mt5-term-api-trade-functions.proto",
    "mt5-term-api-trade-stats.proto",      "mt5-term-api-trading-helper.proto",
    "mt5-term-api-vnc.proto",
};

std::string lower(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

std::vector<const Descriptor*>     g_messages;
std::vector<const EnumDescriptor*> g_enums;

void collectMessage(const Descriptor* d) {
    g_messages.push_back(d);
    for (int i = 0; i < d->enum_type_count(); ++i)   g_enums.push_back(d->enum_type(i));
    for (int i = 0; i < d->nested_type_count(); ++i) collectMessage(d->nested_type(i));
}

void buildIndex() {
    const DescriptorPool* pool = DescriptorPool::generated_pool();
    for (const std::string& name : kProtoFiles) {
        const FileDescriptor* fd = pool->FindFileByName(name);
        if (!fd) continue;   // not linked for some reason — skip quietly
        for (int i = 0; i < fd->message_type_count(); ++i) collectMessage(fd->message_type(i));
        for (int i = 0; i < fd->enum_type_count(); ++i)    g_enums.push_back(fd->enum_type(i));
    }
    auto byName = [](auto* a, auto* b) { return a->full_name() < b->full_name(); };
    std::sort(g_messages.begin(), g_messages.end(), byName);
    std::sort(g_enums.begin(),    g_enums.end(),    byName);
    g_messages.erase(std::unique(g_messages.begin(), g_messages.end()), g_messages.end());
    g_enums.erase(std::unique(g_enums.begin(), g_enums.end()), g_enums.end());
}

// Human-readable proto type for a field (message/enum resolved to their type name).
std::string fieldType(const FieldDescriptor* f) {
    std::string base;
    switch (f->type()) {
        case FieldDescriptor::TYPE_MESSAGE:
        case FieldDescriptor::TYPE_GROUP: base = std::string(f->message_type()->name()); break;
        case FieldDescriptor::TYPE_ENUM:  base = std::string(f->enum_type()->name());    break;
        default:                          base = f->type_name();                         break;  // int64, double, string…
    }
    if (f->is_map()) {
        const FieldDescriptor* k = f->message_type()->field(0);
        const FieldDescriptor* v = f->message_type()->field(1);
        return "map<" + std::string(k->type_name()) + ", " + fieldType(v) + ">";
    }
    if (f->is_repeated())            return "repeated " + base;
    return base;
}

const Descriptor* findMessage(const std::string& q) {
    std::string lq = lower(q);
    const Descriptor* exact = nullptr;
    for (const Descriptor* d : g_messages) {
        if (lower(d->name()) == lq || lower(d->full_name()) == lq) { exact = d; break; }
    }
    return exact;
}

const EnumDescriptor* findEnum(const std::string& q) {
    std::string lq = lower(q);
    for (const EnumDescriptor* e : g_enums)
        if (lower(e->name()) == lq || lower(e->full_name()) == lq) return e;
    return nullptr;
}

void printMessage(const Descriptor* d) {
    std::cout << "\n╔═══ MESSAGE: " << d->name() << " ═══╗\n\n";
    if (d->field_count() == 0) { std::cout << "  (no fields)\n"; return; }
    std::cout << "\U0001F4CB Fields (" << d->field_count() << "):\n\n";
    for (int i = 0; i < d->field_count(); ++i) {
        const FieldDescriptor* f = d->field(i);
        std::ostringstream tag; tag << "#" << f->number();
        std::cout << "  • " << std::left;
        std::cout.width(5);  std::cout << tag.str();
        std::cout.width(42); std::cout << f->name();
        std::cout << ": " << fieldType(f);
        if (const OneofDescriptor* o = f->real_containing_oneof())
            std::cout << "   (oneof " << o->name() << ")";
        std::cout << "\n";
    }
}

void printEnum(const EnumDescriptor* e) {
    std::cout << "\n╔═══ ENUM: " << e->name() << " ═══╗\n\n";
    for (int i = 0; i < e->value_count(); ++i) {
        std::cout << "  " << std::left;
        std::cout.width(46); std::cout << e->value(i)->name();
        std::cout << " = " << e->value(i)->number() << "\n";
    }
}

void cmdList()  { for (const Descriptor* d : g_messages)    std::cout << "  " << d->full_name() << "\n"; }
void cmdEnums() { for (const EnumDescriptor* e : g_enums)   std::cout << "  " << e->full_name() << "\n"; }

void cmdSearch(const std::string& text) {
    std::string lt = lower(text); int n = 0;
    for (const Descriptor* d : g_messages)
        if (lower(d->name()).find(lt) != std::string::npos) { std::cout << "  \U0001F4E6 " << d->full_name() << "\n"; ++n; }
    for (const EnumDescriptor* e : g_enums)
        if (lower(e->name()).find(lt) != std::string::npos) { std::cout << "  \U0001F524 " << e->full_name() << " (enum)\n"; ++n; }
    if (!n) std::cout << "  no types matching \"" << text << "\"\n";
}

void cmdField(const std::string& field) {
    std::string lf = lower(field); int n = 0;
    for (const Descriptor* d : g_messages)
        for (int i = 0; i < d->field_count(); ++i)
            if (lower(d->field(i)->name()) == lf) {
                std::cout << "  \U0001F4E6 " << d->name() << "\n       └─ "
                          << d->field(i)->name() << " : " << fieldType(d->field(i)) << "\n";
                ++n; break;
            }
    if (!n) std::cout << "  no message declares a field named \"" << field << "\"\n";
}

void help() {
    std::cout <<
        "\nCommands (type names are case-insensitive):\n"
        "  list | ls            list all message types\n"
        "  enums                list all enum types\n"
        "  <TypeName>           inspect a message's fields\n"
        "  search <text>        search types by name (alias: find)\n"
        "  field <name>         find messages that declare a field\n"
        "  enum <name>          show an enum's values\n"
        "  help | ?             this help\n"
        "  exit | quit | q      leave\n\n";
}

} // namespace

int main() {
    buildIndex();

    std::cout <<
        "╔════════════════════════╗\n"
        "║   \U0001F50D PROTOBUF INSPECTOR — mt5_term_api    ║\n"
        "║  Explore MT5 gRPC messages & enums offline   ║\n"
        "╚════════════════════════╝\n";
    std::cout << "Indexed " << g_messages.size() << " messages, " << g_enums.size()
              << " enums from the generated code.\n";
    std::cout << "Type 'help' for commands.\n\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;      // EOF (e.g. piped input)

        // trim
        auto a = line.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) continue;
        auto b = line.find_last_not_of(" \t\r\n");
        line = line.substr(a, b - a + 1);

        std::istringstream iss(line);
        std::string cmd; iss >> cmd;
        std::string rest; std::getline(iss, rest);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
        std::string lcmd = lower(cmd);

        if (lcmd == "exit" || lcmd == "quit" || lcmd == "q") break;
        else if (lcmd == "help" || lcmd == "?")               help();
        else if (lcmd == "list" || lcmd == "ls")              cmdList();
        else if (lcmd == "enums")                             cmdEnums();
        else if (lcmd == "search" || lcmd == "find")          cmdSearch(rest);
        else if (lcmd == "field")                             cmdField(rest);
        else if (lcmd == "enum")                              { if (auto* e = findEnum(rest)) printEnum(e);
                                                                else std::cout << "  no enum named \"" << rest << "\"\n"; }
        else {
            // bare token → try message, then enum
            if (const Descriptor* d = findMessage(cmd))        printMessage(d);
            else if (const EnumDescriptor* e = findEnum(cmd))  printEnum(e);
            else std::cout << "  unknown type or command: \"" << cmd << "\"  (try 'search " << cmd << "' or 'help')\n";
        }
    }
    std::cout << "bye.\n";
    return 0;
}
