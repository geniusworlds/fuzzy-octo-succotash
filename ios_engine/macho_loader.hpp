#ifndef MACHO_LOADER_HPP
#define MACHO_LOADER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

// Standard Mach-O 64-bit Constants
constexpr uint32_t MH_MAGIC_64 = 0xfeedfacf;
constexpr uint32_t MH_CIGAM_64 = 0xcffaedfe;

constexpr uint32_t CPU_TYPE_ARM64 = 0x0100000c;
constexpr uint32_t CPU_SUBTYPE_ARM64_ALL = 0x00000000;
constexpr uint32_t CPU_SUBTYPE_ARM64_V8 = 0x00000001;

// Load command types
constexpr uint32_t LC_REQ_DYLD = 0x80000000;
constexpr uint32_t LC_SEGMENT_64 = 0x19;
constexpr uint32_t LC_SYMTAB = 0x02;
constexpr uint32_t LC_DYSYMTAB = 0x0b;
constexpr uint32_t LC_LOAD_DYLINKER = 0x0e;
constexpr uint32_t LC_LOAD_DYLIB = 0x0c;
constexpr uint32_t LC_MAIN = (0x28 | LC_REQ_DYLD);
constexpr uint32_t LC_UNIXTHREAD = 0x05;
constexpr uint32_t LC_CODE_SIGNATURE = 0x1d;
constexpr uint32_t LC_BUILD_VERSION = 0x32;

// Section Protection flags
constexpr uint32_t VM_PROT_NONE = 0x00;
constexpr uint32_t VM_PROT_READ = 0x01;
constexpr uint32_t VM_PROT_WRITE = 0x02;
constexpr uint32_t VM_PROT_EXECUTE = 0x04;

#pragma pack(push, 1)

struct MachHeader64 {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
};

struct LoadCommand {
    uint32_t cmd;
    uint32_t cmdsize;
};

struct SegmentCommand64 {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
};

struct Section64 {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
};

struct EntryPointCommand {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff;
    uint64_t stacksize;
};

struct SymtabCommand {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
};

#pragma pack(pop)

struct MachOSegment {
    std::string name;
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t initprot;
    std::vector<uint8_t> data;
};

class MachOLoader {
public:
    MachOLoader();
    ~MachOLoader();

    bool loadFromBuffer(const uint8_t* buffer, size_t size);
    bool loadFromFile(const std::string& path);

    uint64_t getEntryPoint() const { return entryPoint; }
    uint64_t getPreferredBase() const { return preferredBase; }
    const std::vector<MachOSegment>& getSegments() const { return segments; }
    const std::vector<std::string>& getDylibs() const { return importedDylibs; }
    const std::map<std::string, uint64_t>& getExportedSymbols() const { return symbols; }

    void printBinarySummary() const;

private:
    MachHeader64 header;
    std::vector<MachOSegment> segments;
    std::vector<std::string> importedDylibs;
    std::map<std::string, uint64_t> symbols;
    uint64_t entryPoint;
    uint64_t preferredBase;
};

#endif // MACHO_LOADER_HPP
