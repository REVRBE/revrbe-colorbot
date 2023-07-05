#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")

#include <iostream>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <vector>
#include <thread>
#include <algorithm>
#include "skCrypter.h"

constexpr int threshold = 2; // Smaller value means more specific colors
constexpr int scanDelay = 1; // Delay between each screen scan (in milliseconds)
constexpr int pixelRange = 1; // -1, 0, 1 makes a 3x3 grid
constexpr int delay = 0; // Delay for after-shoot (in milliseconds)
constexpr int afterShootDelay = 325; // Delay for after-shoot (in milliseconds)

bool isButton5Down()
{
    return GetAsyncKeyState(VK_XBUTTON2) & 0x8000;
}

std::pair<bool, POINT> isColorDetected(HDC hdcMem, int width, int height, const std::vector<COLORREF>& targetColors, const std::vector<COLORREF>& excludedColors)
{
    for (const auto& target : targetColors)
    {
        BYTE targetRed = GetRValue(target);
        BYTE targetGreen = GetGValue(target);
        BYTE targetBlue = GetBValue(target);

        // Iterate over a 3x3 region of pixels
        for (int i = -pixelRange; i <= pixelRange; i++)
        {
            for (int j = -pixelRange; j <= pixelRange; j++)
            {
                COLORREF color = GetPixel(hdcMem, width / 2 + i, height / 2 + j);

                BYTE red = GetRValue(color);
                BYTE green = GetGValue(color);
                BYTE blue = GetBValue(color);

                // Check if the color is in the excludedColors list
                if (std::find(excludedColors.begin(), excludedColors.end(), color) != excludedColors.end())
                {
                    continue; // Skip this color
                }

                double distance = sqrt(
                    (red - targetRed) * (red - targetRed) +
                    (green - targetGreen) * (green - targetGreen) +
                    (blue - targetBlue) * (blue - targetBlue));

                if (distance <= threshold)
                {
                    return std::make_pair(true, POINT{ i, j });
                }
            }
        }
    }
    return std::make_pair(false, POINT{ 0, 0 });
}

int main()
{
    int width = 2 * pixelRange + 1; // 3 pixels wide
    int height = 2 * pixelRange + 1; // 3 pixels tall

    HDC hdc = GetDC(NULL);

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(hdcMem, hBitmap);

    std::vector<COLORREF> recordedColors; // Vector to hold the recorded colors
    std::vector<COLORREF> excludedColors; // Vector to hold the excluded colors
    bool recordingStarted = false;
    bool recordingStopped = false;

    POINT lastCursorPosition = { 0, 0 };
    bool shouldSendInputs = false; // Flag to determine if inputs should be sent

    while (true)
    {
        // Check the state of the Insert key
        if (GetAsyncKeyState(VK_INSERT) & 0x8000)
        {
            if (!recordingStarted)
            {
                recordingStarted = true;
                recordedColors.clear();
                std::cout << skCrypt("Recording started. Press INSERT again to stop recording.").decrypt() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            else if (recordingStarted && !recordingStopped)
            {
                recordingStarted = false;
                recordingStopped = true;
                std::cout << skCrypt("Recording stopped. Recorded colors:").decrypt() << std::endl;
                for (const auto& color : recordedColors)
                {
                    BYTE red = GetRValue(color);
                    BYTE green = GetGValue(color);
                    BYTE blue = GetBValue(color);
                    std::cout << skCrypt("Red: ").decrypt() << static_cast<int>(red) << skCrypt(", Green: ").decrypt() << static_cast<int>(green) << skCrypt(", Blue: ").decrypt() << static_cast<int>(blue) << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        else
        {
            if (recordingStopped)
            {
                recordingStopped = false;
            }
        }

        // Pressing the HOME key will add the current pixel color to the excludedColors list
        if (GetAsyncKeyState(VK_HOME) & 0x8000)
        {
            // Exclude colors in a 3x3 region around the central pixel
            for (int i = -pixelRange; i <= pixelRange; i++)
            {
                for (int j = -pixelRange; j <= pixelRange; j++)
                {
                    COLORREF pixelColor = GetPixel(hdcMem, width / 2 + i, height / 2 + j);
                    if (std::find(excludedColors.begin(), excludedColors.end(), pixelColor) == excludedColors.end())
                    {
                        excludedColors.push_back(pixelColor);
                    }

                    // Remove color from the good list if it's there
                    auto it = std::find(recordedColors.begin(), recordedColors.end(), pixelColor);
                    if (it != recordedColors.end())
                    {
                        recordedColors.erase(it);
                        std::cout << skCrypt("Color removed from the good list.").decrypt() << std::endl;
                    }
                }
            }

            std::cout << skCrypt("Colors added to the exclusion list.").decrypt() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        // Check the distance between the last and current cursor position
        int cursorDistance = static_cast<int>(std::sqrt(std::pow(cursorPos.x - lastCursorPosition.x, 2) + std::pow(cursorPos.y - lastCursorPosition.y, 2)));

        // Calculate the cursor speed
        double cursorSpeed = static_cast<double>(cursorDistance) / scanDelay;

        lastCursorPosition = cursorPos;

        POINT lastDetectedPos = { 0, 0 }; // Last detected position of the target color
        int extraSteps = 3; // Number of additional steps towards the target color

        if (isButton5Down())
        {
            BitBlt(hdcMem, 0, 0, width, height, hdc, cursorPos.x - width / 2, cursorPos.y - height / 2, SRCCOPY);

            if (recordingStarted)
            {
                COLORREF detectedColor = GetPixel(hdcMem, width / 2, height / 2); // Get color of the central pixel
                if (std::find(recordedColors.begin(), recordedColors.end(), detectedColor) == recordedColors.end())
                {
                    recordedColors.push_back(detectedColor);
                    std::cout << skCrypt("Color recorded.").decrypt() << std::endl;
                }
            }

            auto [colorDetected, detectedPos] = isColorDetected(hdcMem, width, height, recordedColors, excludedColors);

            if (colorDetected && !recordingStarted)
            {
                std::cout << skCrypt("Target color detected within the rectangle.").decrypt() << std::endl;

                // Prediction logic
                POINT movementDirection = { detectedPos.x - lastDetectedPos.x, detectedPos.y - lastDetectedPos.y };
                detectedPos.x += movementDirection.x * extraSteps;
                detectedPos.y += movementDirection.y * extraSteps;
                lastDetectedPos = detectedPos;
                
                if (shouldSendInputs)
                {
                    // Relative cursor movement
                    INPUT inputMove = { 0 };
                    inputMove.type = INPUT_MOUSE;
                    inputMove.mi.dwFlags = MOUSEEVENTF_MOVE;
                    inputMove.mi.dx = detectedPos.x;
                    inputMove.mi.dy = detectedPos.y;
                    SendInput(1, &inputMove, sizeof(INPUT));

                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                    // Mouse click
                    INPUT inputClick = { 0 };
                    inputClick.type = INPUT_MOUSE;
                    inputClick.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &inputClick, sizeof(INPUT));

                    std::this_thread::sleep_for(std::chrono::milliseconds(afterShootDelay));

                    inputClick.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &inputClick, sizeof(INPUT));
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(scanDelay));
    }

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return 0;
}
