#include "macho_loader.hpp"
#include <fstream>
#include <cstring>
#include <iomanip>

MachOLoader::MachOLoader() : entryPoint(0), preferredBase(0) {
    std::memset(&header, 0, sizeof(header));
}

MachOLoader::~MachOLoader() {}

bool MachOLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[MachO] Failed to open file: " << path << std::endl;
        return false;
    }
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "[MachO] Failed to read file data: " << path << std::endl;
        return false;
    }
    return loadFromBuffer(buffer.data(), size);
}

bool MachOLoader::loadFromBuffer(const uint8_t* buffer, size_t size) {
    if (size < sizeof(MachHeader64)) {
        std::cerr << "[MachO] Error: Buffer too small for Mach-O 64 header." << std::endl;
        return false;
    }

    std::memcpy(&header, buffer, sizeof(MachHeader64));

    if (header.magic != MH_MAGIC_64) {
        if (header.magic == MH_CIGAM_64) {
            std::cerr << "[MachO] Error: Big-endian Mach-O 64 is not supported." << std::endl;
        } else {
            std::cerr << "[MachO] Error: Invalid Mach-O magic: 0x" 
                      << std::hex << header.magic << std::dec << std::endl;
        }
        return false;
    }

    if (header.cputype != CPU_TYPE_ARM64) {
        std::cerr << "[MachO] Warning: CPU Type is not ARM64 (Expected 0x0100000C, got 0x" 
                  << std::hex << header.cputype << std::dec << ")." << std::endl;
    }

    size_t offset = sizeof(MachHeader64);
    segments.clear();
    importedDylibs.clear();
    symbols.clear();

    preferredBase = 0;
    entryPoint = 0;

    for (uint32_t i = 0; i < header.ncmds; ++i) {
        if (offset + sizeof(LoadCommand) > size) {
            std::cerr << "[MachO] Error: Load commands exceed buffer size." << std::endl;
            return false;
        }

        const LoadCommand* cmd = reinterpret_cast<const LoadCommand*>(buffer + offset);
        if (cmd->cmdsize == 0 || offset + cmd->cmdsize > size) {
            std::cerr << "[MachO] Error: Corrupted load command size." << std::endl;
            return false;
        }

        switch (cmd->cmd) {
            case LC_SEGMENT_64: {
                if (cmd->cmdsize >= sizeof(SegmentCommand64)) {
                    const SegmentCommand64* seg = reinterpret_cast<const SegmentCommand64*>(cmd);
                    MachOSegment s;
                    s.name = std::string(seg->segname, strnlen(seg->segname, 16));
                    s.vmaddr = seg->vmaddr;
                    s.vmsize = seg->vmsize;
                    s.fileoff = seg->fileoff;
                    s.filesize = seg->filesize;
                    s.initprot = seg->initprot;

                    if (s.filesize > 0 && s.fileoff + s.filesize <= size) {
                        s.data.resize(s.filesize);
                        std::memcpy(s.data.data(), buffer + s.fileoff, s.filesize);
                    }

                    if (s.name == "__TEXT" && preferredBase == 0) {
                        preferredBase = s.vmaddr;
                    }

                    segments.push_back(s);
                }
                break;
            }
            case LC_MAIN: {
                if (cmd->cmdsize >= sizeof(EntryPointCommand)) {
                    const EntryPointCommand* ep = reinterpret_cast<const EntryPointCommand*>(cmd);
                    entryPoint = preferredBase + ep->entryoff;
                }
                break;
            }
            case LC_LOAD_DYLIB: {
                // String offset is relative to the load command start
                struct DylibCommand {
                    uint32_t cmd;
                    uint32_t cmdsize;
                    uint32_t name_offset;
                    uint32_t timestamp;
                    uint32_t current_version;
                    uint32_t compatibility_version;
                };
                if (cmd->cmdsize >= sizeof(DylibCommand)) {
                    const DylibCommand* dylibCmd = reinterpret_cast<const DylibCommand*>(cmd);
                    if (dylibCmd->name_offset < cmd->cmdsize) {
                        const char* dylibName = reinterpret_cast<const char*>(buffer + offset + dylibCmd->name_offset);
                        importedDylibs.push_back(std::string(dylibName));
                    }
                }
                break;
            }
            case LC_SYMTAB: {
                if (cmd->cmdsize >= sizeof(SymtabCommand)) {
                    const SymtabCommand* symtab = reinterpret_cast<const SymtabCommand*>(cmd);
                    if (symtab->stroff < size && symtab->symoff < size) {
                        const char* strtable = reinterpret_cast<const char*>(buffer + symtab->stroff);
                        struct Nlist64 {
                            uint32_t n_strx;
                            uint8_t  n_type;
                            uint8_t  n_sect;
                            uint16_t n_desc;
                            uint64_t n_value;
                        };
                        const Nlist64* nlist = reinterpret_cast<const Nlist64*>(buffer + symtab->symoff);
                        for (uint32_t s = 0; s < symtab->nsyms; ++s) {
                            if (nlist[s].n_strx < symtab->strsize) {
                                const char* symName = strtable + nlist[s].n_strx;
                                if (symName && symName[0] != '\0') {
                                    symbols[symName] = nlist[s].n_value;
                                }
                            }
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
        offset += cmd->cmdsize;
    }

    if (entryPoint == 0 && preferredBase != 0) {
        entryPoint = preferredBase; // Fallback to segment start if LC_MAIN wasn't present
    }

    return true;
}

void MachOLoader::printBinarySummary() const {
    std::cout << "========== Mach-O 64-bit Binary Details ==========" << std::endl;
    std::cout << "Magic: 0x" << std::hex << header.magic 
              << " | CPU: ARM64 (0x" << header.cputype << ")" << std::dec << std::endl;
    std::cout << "Number of Load Commands: " << header.ncmds << std::endl;
    std::cout << "Preferred Base Address: 0x" << std::hex << preferredBase << std::dec << std::endl;
    std::cout << "Entry Point Address:    0x" << std::hex << entryPoint << std::dec << std::endl;
    
    std::cout << "\nSegments:" << std::endl;
    for (const auto& seg : segments) {
        std::cout << "  Segment: " << std::left << std::setw(12) << seg.name 
                  << " VMAddr: 0x" << std::hex << std::setw(10) << seg.vmaddr
                  << " VMSize: 0x" << std::setw(8) << seg.vmsize
                  << " FileOff: 0x" << std::setw(8) << seg.fileoff
                  << " Prot: " << seg.initprot << std::dec << std::endl;
    }

    std::cout << "\nImported Dynamic Libraries (Dylibs):" << std::endl;
    for (const auto& lib : importedDylibs) {
        std::cout << "  - " << lib << std::endl;
    }
    std::cout << "===================================================" << std::endl;
}
