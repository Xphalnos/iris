#include <vector>
#include <string>
#include <cctype>
#include <cstdlib>

#include "iris.hpp"

#include "shared/ata/isif.h"

#include "res/IconsMaterialSymbols.h"
#include "portable-file-dialogs.h"

namespace iris {

enum : int {
    IMAGE_FMT_RAW,
    IMAGE_FMT_ISIF
};

const char* image_format_names[] = {
    "RAW",
    "ISIF"
};

void create_raw_image(const char* path, uint64_t size) {
    FILE* file = fopen(path, "wb");

    void* buf = malloc(0x100000);

    memset(buf, 0, 0x100000);

    for (uint64_t written = 0; written < size; written += 0x100000) {
        fwrite(buf, 1, 0x100000, file);
    }

    free(buf);

    fclose(file);
}

std::string get_file_size_string(int format, uint64_t size) {
    uint64_t total_size = 0;

    if (format == IMAGE_FMT_ISIF) {
        uint64_t block_count = size / 512;

        // BAT size
        total_size = block_count * sizeof(uint64_t);
    } else {
        total_size = size;
    }

    char buf[128];

    if (total_size >= 0x40000000) {
        sprintf(buf, "%.1f GiB", (float)total_size / 0x40000000ull);
    } else if (total_size >= 0x100000) {
        sprintf(buf, "%.1f MiB", (float)total_size / 0x100000ull);
    } else if (total_size >= 0x400) {
        sprintf(buf, "%.1f KiB", (float)total_size / 0x400ull);
    } else {
        sprintf(buf, "%llu B", total_size);
    }

    return std::string(buf);
}

void show_hdd_tool(iris::instance* iris) {
    using namespace ImGui;

    static int image_format = IMAGE_FMT_ISIF;
    static int size_add = 0;
    static bool assign = true;

    if (imgui::BeginEx("HDD Tool", &iris->show_hdd_tool)) {
        if (BeginTabBar("##hddtooltabs")) {
            if (BeginTabItem("Create")) {
                Text("Image format");
                Combo("##image_format", &image_format, image_format_names, IM_ARRAYSIZE(image_format_names));

                Text("Size (GiB)");

                std::string size_hint = std::to_string((0xa00000000 + (0x500000000 * size_add)) / 0x40000000ull);

                if (BeginCombo("##sizepreset", size_hint.c_str(), 0)) {
                    for (int i = 0; i < 4; i++) {
                        std::string str = std::to_string((0xa00000000 + (0x500000000 * i)) / 0x40000000ull);

                        if (Selectable(str.c_str())) {
                            size_add = i;
                        }
                    }

                    EndCombo();
                }

                TextDisabled("Estimated size     "); SameLine(0.0, 0.0);
                Text(get_file_size_string(image_format, 0xa00000000 + (0x500000000 * size_add)).c_str());

                PushStyleVarY(ImGuiStyleVar_FramePadding, 2.0F);
                Checkbox("Attach to PS2", &assign);
                PopStyleVar();

                if (Button("Create")) {
                    std::string default_path = iris->pref_path;
                    
                    if (image_format == IMAGE_FMT_RAW) {
                        default_path += "hdd.raw";
                    } else {
                        default_path += "hdd.isif";
                    }

                    auto f = pfd::save_file("Save HDD image", default_path, {
                        "ISIF Image (*.isif)", "*.isif",
                        "RAW Image (*.raw *.bin)", "*.raw *.bin",
                        "All Files (*.*)", "*"
                    });

                    while (!f.ready());

                    if (f.result().size()) {
                        if (image_format == IMAGE_FMT_RAW) {
                            create_raw_image(f.result().c_str(), 0xa00000000 + (0x500000000 * size_add));
                        } else {
                            uint64_t block_count = (0xa00000000 + (0x500000000 * size_add)) / 512;

                            isif_create_image(f.result().c_str(), block_count, 512, 1, 0, nullptr, 0);
                        }
                    }

                    if (assign) {
                        iris->hdd_path = f.result();
                    }
                }

                EndTabItem();
            }

            if (BeginTabItem("Convert")) {
                Text("This tool is a work in progress and doesn't do anything yet.");
                EndTabItem();
            }

            EndTabBar();
        }
    } End();
}

}