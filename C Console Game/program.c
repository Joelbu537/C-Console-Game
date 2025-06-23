#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <Windows.h>
#include <stringapiset.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#define FONT_SIZE 30
#define PADDING 10

struct Item;
typedef struct Item {
	byte weight;
	char* name;
} Item;
struct Text;
typedef struct Text {
	SDL_Rect rect;
	SDL_Surface* surface;
	SDL_Texture* texture;
	char* text; // Optional: Store the text for reference
} Text;


// Zwei Texte, einer für Konsole, einer für Text
// Erster Text enthält \n, um Zeilenumbrüche zu erzeugen


void ShowUtf8MessageBox(const char* utf8Message, const wchar_t* title) {
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, NULL, 0);
    wchar_t* wideMessage = malloc(size * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, wideMessage, size);
    MessageBoxW(NULL, wideMessage, title, MB_OK | MB_ICONERROR);
    free(wideMessage);
}

Text GetRect(SDL_Renderer* renderer, TTF_Font* font, char* text) {
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, (SDL_Color) { 255, 255, 255, 255 });
	if (textSurface == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_RenderText_Solid Error: %s", TTF_GetError());
		ShowUtf8MessageBox(TTF_GetError(), L"TTF_RenderText_Solid Error");
		return (Text) { 0 };
	}
	SDL_Texture* TitleTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	if (TitleTexture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
		SDL_FreeSurface(textSurface);
		ShowUtf8MessageBox(SDL_GetError(), L"SDL_CreateTextureFromSurface Error");
		return (Text) { 0 };
	}
	int titleWidth, titleHeight;
	TTF_SizeText(font, text, &titleWidth, &titleHeight);
	SDL_Rect titleRect = { 0, 0, titleWidth, titleHeight };

	Text result = { titleRect, textSurface, TitleTexture };
	result.text = malloc(strlen(text) + 1);
	if (result.text == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Memory allocation failed for text");
		SDL_DestroyTexture(TitleTexture);
		SDL_FreeSurface(textSurface);
		return (Text) { 0 };
	}
	strcpy(result.text, text);
	return result;
}

                     // Instanz           // Immer NULL            // Args-string   //  Fenstermodus
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

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

	unsigned short maxLines = height / FONT_SIZE;
	unsigned short leftoverHeight = height % FONT_SIZE;
	Text* textLines = malloc(maxLines * sizeof(Text));
	if (textLines == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Memory allocation failed for text lines");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		TTF_CloseFont(fontCascadia);
		free(fontPath);
		SDL_free(basePath);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}
	for (unsigned short i = 0; i < maxLines; i++)
	{
		textLines[i].rect.x = PADDING;
		textLines[i].rect.y = i * FONT_SIZE;
		textLines[i].rect.w = width - (PADDING * 2); // 10px padding on each side
		textLines[i].rect.h = FONT_SIZE;
		textLines[i].surface = NULL;
		textLines[i].texture = NULL;
	}
	SDL_Log("Text lines initialized: %d lines", maxLines);

	int maxWidth = width - PADDING * 2;
	int charWidth;
	TTF_SizeText(fontCascadia, "A", &charWidth, NULL);
	int maxCharactersPerLine = maxWidth / charWidth;
	SDL_Log("Max characters per line: %d", maxCharactersPerLine);

	/* Debug	*/
	for (unsigned short i = 0; i < maxLines - 3; i++) {
		char buffer[64];
		SDL_snprintf(buffer, sizeof(buffer), "Line %d", i + 1);
		Text text = GetRect(renderer, fontCascadia, buffer);
		textLines[i].surface = text.surface;
		textLines[i].texture = text.texture;
		textLines[i].rect.w = text.rect.w;
		if (textLines[i].surface == NULL || textLines[i].texture == NULL) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text line %d", i);
			free(textLines);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			TTF_CloseFont(fontCascadia);
			free(fontPath);
			SDL_free(basePath);
			TTF_Quit();
			SDL_Quit();
			return 1;
		}
	}
	// DEBUG END
	
	bool applicationRunning = true;
	bool isMenu = true;
	SDL_Event event;

	char* titleText = "Press [ENTER] to continue...";
	SDL_Surface* TitleSurface = TTF_RenderText_Solid(fontCascadia, titleText, (SDL_Color) { 255, 255, 255, 255 });
	SDL_Texture* TitleTexture = SDL_CreateTextureFromSurface(renderer, TitleSurface);
	if (TitleTexture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
		SDL_FreeSurface(TitleSurface);
		applicationRunning = false;
	}
	int titleWidth, titleHeight;
	TTF_SizeText(fontCascadia, titleText, &titleWidth, &titleHeight);
	int titleX, titleY;
	titleX = (width - titleWidth) / 2;
	titleY = (height - titleHeight) / 2;
	SDL_Rect titleRect = { titleX, titleY, titleWidth, titleHeight };
	SDL_Log("Title rendering prepared");

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
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					applicationRunning = false;
				}
				else if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						SDL_Log("User paused the game.");
						isMenu = true;
					}
				}
			}
			for (unsigned short i = 0; i < maxLines; i++) {
				if (textLines[i].surface != NULL && textLines[i].texture != NULL) {
					SDL_RenderCopy(renderer, textLines[i].texture, NULL, &textLines[i].rect);
				}
			}
		}
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_CloseFont(fontCascadia);
	free(fontPath);
	SDL_free(basePath);
	TTF_Quit();
	SDL_Log("Application exited successfully.");
	SDL_Quit();


	return 0;
}