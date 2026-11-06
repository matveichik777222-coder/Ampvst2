# AmpForge — гитарный VST3 (JUCE 8 + CMake)

Цепочка: Input → Педали (4 слота) → Усилитель → Кабинет/IR → Эффекты (3 слота) → Output.
Тип эффекта выбирается в каждом слоте, порядок меняется кнопками < > (меняют слоты местами).

## Сборка (Windows 11)
Нужны: Visual Studio 2022 (Desktop C++), CMake 3.22+, Git.

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target AmpForge_VST3 AmpForge_Standalone

Результат: `build/AmpForge_artefacts/Release/VST3/AmpForge.vst3` (папка-бандл) и Standalone `.exe`.
Скопируйте `AmpForge.vst3` в `C:\Program Files\Common Files\VST3\`, затем пересканируйте плагины в FL Studio / Reaper.

Без локальной установки: залейте папку в GitHub — `.github/workflows/build.yml` соберёт Windows-артефакт сам.

## Структура
- `Source/DSP` — биквады, SVF, ADAA-вейвшейперы, линия задержки
- `Source/Amps` — 7 моделей усилителей (преамп на асимметричных триодных каскадах, тон-стек, sag, power amp, presence, oversampling 2x/4x)
- `Source/Cabinet` — свёртка (juce::dsp::Convolution), встроенные синтезированные IR (1x12/2x12/4x12, 5 динамиков, 4 микрофона, позиция, дистанция), загрузка своих WAV, HP/LP
- `Source/Effects` — 18 педалей/эффектов
- `Source/Presets` — 14 заводских пресетов (Clean, Blues, Rock, Metal, Shoegaze, Indie, Ambient) + пользовательские в `%APPDATA%\AmpForge\Presets`
- `Source/UI` — тёмный интерфейс, вкладки Pedals / Amp / Cabinet / Effects

## Лицензия
JUCE — AGPLv3/коммерческая лицензия; для распространения VST3 учитывайте условия JUCE.
