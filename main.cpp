#include <optional>
#include <fstream>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>

#if defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#else

#include <nfd.h>

#endif

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

struct Context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    // Declare the texture globally
    SDL_Texture *backgroundTexture = nullptr;
    bool done = false;
    std::optional<std::vector<char>> reference_image_data;
};
Context context;


void on_reference_image_load() {
    // Load the image
    int width, height, channels;
    unsigned char *imageData = stbi_load_from_memory(reinterpret_cast<stbi_uc *>(context.reference_image_data->data()),
                                                     static_cast<int>(context.reference_image_data->size()), &width,
                                                     &height, &channels, STBI_rgb_alpha);
    if (imageData == nullptr) {
        SDL_Log("Unable to load image with stb_image!\n");
    } else {
        // Unload the old texture if it exists
        if (context.backgroundTexture != nullptr) {
            SDL_DestroyTexture(context.backgroundTexture);
            context.backgroundTexture = nullptr;
        }

        SDL_SetWindowSize(context.window, width, height);
        // Create texture from the loaded data
        context.backgroundTexture = SDL_CreateTexture(context.renderer, SDL_PIXELFORMAT_RGBA32,
                                                      SDL_TEXTUREACCESS_STATIC, width, height);
        if (context.backgroundTexture == nullptr) {
            SDL_Log("Unable to create texture! SDL Error: %s\n", SDL_GetError());
        } else {
            SDL_UpdateTexture(context.backgroundTexture, nullptr, imageData, width * sizeof(uint32_t));
        }

        // Free the image data
        stbi_image_free(imageData);
    }
    context.reference_image_data = std::nullopt;
}

#if defined(EMSCRIPTEN)
extern "C" {
// Declare the callback function as extern "C" to prevent name mangling
EMSCRIPTEN_KEEPALIVE void fileRead(int data, int length) {
    context.reference_image_data = std::vector<char>(reinterpret_cast<char*>(data), reinterpret_cast<char*>(data + length));
}
}

EM_JS(void, jsOpenDialog, (), {
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/jpeg,image/png';

    input.onchange = function(e) {
        var file = e.target.files[0];
        var reader = new FileReader();

        reader.onload = function(e) {
            var contents = new Uint8Array(e.target.result);
            var length = contents.length;
            var data = Module._malloc(length);

            Module.HEAPU8.set(contents, data);
            Module.ccall('fileRead', null, ['number', 'number'], [data, length]);

            Module._free(data);
        };

        reader.readAsArrayBuffer(file);
    };

    input.click();
});
#endif

void main_loop() {
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            context.done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(context.window))
            context.done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (context.show_demo_window)
        ImGui::ShowDemoWindow(&context.show_demo_window);

    if (context.reference_image_data) {
        on_reference_image_load();
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin(
                "Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window",
                        &context.show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &context.show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button("Load Reference Image")) {
#if defined(EMSCRIPTEN)
            jsOpenDialog();
#else
            nfdchar_t *path = nullptr;
            if (NFD_OpenDialog("png,jpg,jpeg", nullptr, &path) != NFD_OKAY)
                SDL_Log("Could not open dialog");
            else {
                std::string filePath(path);
                free(path);

                std::ifstream file(filePath, std::ios::binary | std::ios::ate);
                if (!file) {
                    SDL_Log("Could not open File");
                } else {
                    const auto fileSize = file.tellg();
                    file.seekg(0, std::ios::beg);

                    std::vector<char> fileData(fileSize);
                    if (!file.read(fileData.data(), fileSize)) {
                        SDL_Log("Could not read File");
                    } else {
                        context.reference_image_data = fileData;
                    }
                }
            }
#endif
        }

        if (ImGui::Button(
                "Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (context.show_another_window) {
        ImGui::Begin("Another Window",
                     &context.show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            context.show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    SDL_RenderSetScale(context.renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
    SDL_RenderClear(context.renderer);
    if (context.backgroundTexture != nullptr)
        SDL_RenderCopy(context.renderer, context.backgroundTexture, nullptr, nullptr);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(context.renderer);
}

int main(int, char **) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    context.window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (context.renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    //SDL_Log("Current SDL_Renderer: %s", info.name);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(context.window, context.renderer);
    ImGui_ImplSDLRenderer2_Init(context.renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Main loop
#if EMSCRIPTEN
    emscripten_set_main_loop(main_loop, -1, 1);
#else
    while (!context.done)
        main_loop();
#endif

    // Cleanup
    if (context.backgroundTexture != nullptr)
        SDL_DestroyTexture(context.backgroundTexture);
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();

    return 0;
}
