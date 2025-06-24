#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <Windows.h>
#include <stringapiset.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#define FONT_SIZE 30
#define PADDING 20
#define MIN_REQUIRED_LINES 5

static char* ConsoleText;
static char* ConsoleInput;
static size_t ConsoleTextMaxSize;
#define CONSOLE_INPUT_MAX_SIZE 128


// Zwei Texte, einer für Konsole, einer für Text
// Erster Text enthält \n, um Zeilenumbrüche zu erzeugen


void ShowUtf8MessageBox(const char* utf8Message, const wchar_t* title) {
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, NULL, 0);
    wchar_t* wideMessage = malloc(size * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, wideMessage, size);
    MessageBoxW(NULL, wideMessage, title, MB_OK | MB_ICONERROR);
    free(wideMessage);
}

int WriteText(char* text) {
	if (text == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Text pointer is NULL");
		return -1;
	}
	if (ConsoleText == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ConsoleText was not initialised!");
		return -1;
	}
	size_t console_text_used_size = strlen(ConsoleText);
	if (strlen(text) + console_text_used_size > ConsoleTextMaxSize) {
		SDL_Log("Genutzt :%zu  Eingabe: %zu,  Gesamt: %zu",
			console_text_used_size, strlen(text) + console_text_used_size, ConsoleTextMaxSize);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Action would result in a bufer overflow.");
		return -1;
	}
	strcat_s(ConsoleText, ConsoleTextMaxSize, text);
	return 0;
}
int FreeText() {
	if (ConsoleText == NULL) {
		return -1;
	}
	ConsoleText[0] = '\0';
	return 0;
}

                     // Instanz           // Immer NULL            // Args-string   //  Fenstermodus
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	bool applicationRunning = true;
	bool isMenu = true;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
		MessageBox(NULL, SDL_GetError(), "SDL_Init Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	SDL_Log("SDL Version %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	if (TTF_Init() != 0) {
		MessageBox(NULL, TTF_GetError(), "TTF_Init Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	SDL_Log("SDL_ttf Version %d.%d.%d", TTF_MAJOR_VERSION, TTF_MINOR_VERSION, TTF_PATCHLEVEL);

	char* basePath = SDL_GetBasePath();
	if (basePath == NULL) {
		ShowUtf8MessageBox(SDL_GetError(), L"SDL_GetBasePath Error");
		return 1;
	}
	else {
		SDL_Log("Base Path: %s", basePath);
	}
	size_t basePathSize = strlen(basePath) + 32;
	char* fontPath = malloc(basePathSize);
	if (fontPath == NULL) {
		MessageBox(NULL, L"Memory allocation failed for font path", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	SDL_snprintf(fontPath, basePathSize, "%s%s", basePath, "CascadiaMono-Regular.ttf");
	SDL_Log("Font Path: %s", fontPath);
	TTF_Font* fontCascadia = TTF_OpenFont(fontPath, FONT_SIZE);
	if (fontCascadia == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_OpenFont Error: %s", TTF_GetError());
		ShowUtf8MessageBox(TTF_GetError(), L"TTF_OpenFont Error");
		return 1;
	}
	SDL_Log("Font loaded successfully: %s", fontPath);

	SDL_Window* window = SDL_CreateWindow("Console", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (window == NULL) {
		ShowUtf8MessageBox(SDL_GetError(), L"SDL_CreateWindow Error");
		return 1;
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		ShowUtf8MessageBox(SDL_GetError(), L"SDL_CreateRenderer Error");
		SDL_DestroyWindow(window);
		return 1;
	}
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	SDL_Log("Window Size: %d x %d", width, height);

	// Calculate disply information
	unsigned short maxLines = height / FONT_SIZE;
	unsigned short maxUsableLines = maxLines - 3;
	if (maxUsableLines < MIN_REQUIRED_LINES) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Not enough lines available for text rendering. Minimum required: %d, Available: %d", MIN_REQUIRED_LINES, maxUsableLines);
		ShowUtf8MessageBox("Not enough lines available for text rendering. Minimum required: 5, Available: less than 5", L"Error");
		applicationRunning = false;
	}
	unsigned short leftoverHeight = height % FONT_SIZE;
	int charWidth = 0;
	TTF_SizeText(fontCascadia, "A", NULL, &charWidth);
	unsigned short maxCharsPerLine = width / charWidth;
	ConsoleTextMaxSize = maxCharsPerLine * maxUsableLines + 64;
	ConsoleText = calloc(ConsoleTextMaxSize, 1);
	ConsoleInput = calloc(CONSOLE_INPUT_MAX_SIZE, 1);
	
	SDL_Event event;

	char* titleText = "Press [ENTER] to continue...";
	SDL_Surface* TitleSurface = TTF_RenderText_Solid(fontCascadia, titleText, (SDL_Color) { 255, 255, 255, 255 });
	SDL_Texture* TitleTexture = SDL_CreateTextureFromSurface(renderer, TitleSurface);
	if (TitleTexture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
		SDL_FreeSurface(TitleSurface);
		applicationRunning = false;
	}
	SDL_Rect titleRect = { (width - TitleSurface->w) / 2, (height - TitleSurface->h) / 2, TitleSurface->w, TitleSurface->h };
	SDL_Rect inputPanelRect = { 0, FONT_SIZE * maxUsableLines, width, FONT_SIZE * 3 };
	SDL_Log("Title rendering prepared");
	if (WriteText("Hallo Welt!\nDas ist ein Testtext!\n") != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Something went wrong.");
	}

	if (ConsoleInput != NULL) {
		strcpy_s(ConsoleInput, CONSOLE_INPUT_MAX_SIZE, "> ");
	}
	else {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ConsoleInput was not initialised!");
	}

	SDL_StartTextInput();
	while (applicationRunning) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		if (isMenu) {
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					applicationRunning = false;
				}
				else if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
						isMenu = false;
						SDL_Log("Menu exited, starting game...");
					}
					else if (event.key.keysym.sym == SDLK_ESCAPE) {
						SDL_Log("User initiated exit.");
						applicationRunning = false;
					}
				}
			}
			SDL_RenderCopy(renderer, TitleTexture, NULL, &titleRect);
		}
		else {
			// Handle input
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					applicationRunning = false;
				}
			     if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						SDL_Log("User paused the game.");
						isMenu = true;
					}
					if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(ConsoleInput) > 2) {
						SDL_Log("Backspace pressed");
					    ConsoleInput[strlen(ConsoleInput) - 1] = '\0';
					}
					if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
						SDL_Log("Enter pressed");
						if (strlen(ConsoleInput) > 2) {
							char* refinedInput = calloc(CONSOLE_INPUT_MAX_SIZE, 1);
							strcat_s(refinedInput, CONSOLE_INPUT_MAX_SIZE, ConsoleInput);
							strcat_s(refinedInput, CONSOLE_INPUT_MAX_SIZE, "\n");
							if (WriteText(refinedInput) != 0) {
								SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to write text to console.");
							}
							strcpy_s(ConsoleInput, CONSOLE_INPUT_MAX_SIZE, "> ");
							free(refinedInput);
						}
					}
				}
				if (event.type == SDL_TEXTINPUT) {
					size_t free = CONSOLE_INPUT_MAX_SIZE - strlen(ConsoleInput) - 1;
					if (strlen(event.text.text) <= free) {
						strcat_s(ConsoleInput, CONSOLE_INPUT_MAX_SIZE, event.text.text);
					}
					else {
						SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Input overflow");
					}

				}
			}
			// Draw ConsoleText
			char* copy = _strdup(ConsoleText); // Dupliziert ConsoleText
			if (copy) {
				char* context = NULL;
				char* line = strtok_s(copy, "\n", &context);
				int y = PADDING;
				while (line) {
					SDL_Surface* surf = TTF_RenderText_Solid(fontCascadia, line, (SDL_Color) {255, 255, 255, 255});
					SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
					SDL_Rect rect = { PADDING, y, surf->w, surf->h };
					SDL_RenderCopy(renderer, tex, NULL, &rect);
					SDL_FreeSurface(surf);
					SDL_DestroyTexture(tex);
					y += FONT_SIZE;
					line = strtok_s(NULL, "\n", &context);
				}
				free(copy);
			}
			
			// Overflow-Text verstecken
			SDL_RenderDrawRect(renderer, &inputPanelRect);

			//Draw ConsoleInput
			if (strlen(ConsoleInput) > 0) {
				SDL_Surface* inputSurf = TTF_RenderText_Solid(fontCascadia, ConsoleInput, (SDL_Color) { 255, 255, 255, 255 });
				SDL_Texture* inputTex = SDL_CreateTextureFromSurface(renderer, inputSurf);
				SDL_Rect inputRect = { PADDING, (maxLines - 2) * FONT_SIZE, inputSurf->w, inputSurf->h };
				SDL_RenderCopy(renderer, inputTex, NULL, &inputRect);
			}
		}
		SDL_RenderPresent(renderer);
	}

	SDL_StopTextInput();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_CloseFont(fontCascadia);
	free(fontPath);
	SDL_free(basePath);
	TTF_Quit();
	SDL_Log("Application exited successfully.");
	SDL_Quit();

	free(ConsoleText);
	free(ConsoleInput);

	return 0;
}