# Developer Documentation
(slop)

## Project Overview

**Tape Looper** is a JUCE-based multitrack audio looper application with support for AI-powered sound generation. It provides three different frontends (Basic, Text2Sound, and VampNet) that share a common audio engine architecture.

### Key Features
- Multiple independent looper tracks (configurable, default 8)
- Real-time recording and playback
- Variable-speed playback (0.25x to 4.0x)
- AI integration via Gradio for text-to-sound and VampNet
- Cross-platform (macOS, Windows, Linux)

---

## Architecture Overview

The application follows a **Model-View-Controller (MVC)** pattern with clear separation between:
- **Engine Layer**: Audio processing and looper logic
- **Frontend Layer**: UI and user interactions
- **Integration Layer**: External services (Gradio HTTP client)

```
┌─────────────────────────────────────────────────┐
│              Application Entry                   │
│  Main.cpp → StartupDialog → Frontend Selection  │
└────────────────┬────────────────────────────────┘
                 │
      ┌──────────┴──────────┐
      │                     │
┌─────▼─────┐      ┌────────▼────────┐
│  Engine   │      │    Frontend     │
│   Layer   │◄─────┤     Layer       │
└───────────┘      └─────────────────┘
      │                     │
      │            ┌────────▼────────┐
      │            │  Shared UI      │
      │            │  Components     │
      │            └─────────────────┘
      │
┌─────▼──────────┐
│  Integration   │
│     Layer      │
└────────────────┘
```

---

## Directory Structure

### `/Source/`
Main application source code

#### `/Source/Engine/`
Core audio processing engine (frontend-agnostic)

- **`MultiTrackLooperEngine.h/cpp`**
  - Top-level engine coordinating multiple tracks
  - Implements `juce::AudioIODeviceCallback` for real-time audio
  - Manages `juce::AudioDeviceManager`
  - Provides track synchronization
  - **Key Methods:**
    - `audioDeviceIOCallbackWithContext()`: Real-time audio callback
    - `getTrack(index)`: Access individual track state
    - `syncAllTracks()`: Align all track playheads

- **`LooperTrackEngine.h/cpp`**
  - Single track processing logic
  - Contains `TrackState` struct with all track data
  - Handles record/playback state transitions
  - **Key Methods:**
    - `processBlock()`: Process audio for one track
    - `initialize()`: Set up buffers based on sample rate
    - `loadFromFile()`: Load audio file into track

- **`TapeLoop.h/cpp`**
  - Audio buffer container
  - Thread-safe buffer management
  - **Key Members:**
    - `buffer`: Vector holding audio samples
    - `recordedLength`: Atomic counter for recorded samples
    - `hasRecorded`: Flag indicating if track has content

- **`LooperWriteHead.h/cpp`**
  - Recording logic
  - Handles writing incoming audio to buffer
  - Manages recording state transitions
  - **Key Methods:**
    - `processBlock()`: Write input audio to tape loop

- **`LooperReadHead.h/cpp`**
  - Playback logic
  - Variable-speed playback with interpolation
  - Position tracking
  - **Key Members:**
    - `playbackSpeed`: Speed multiplier (0.25x - 4.0x)
    - `position`: Current playback position
  - **Key Methods:**
    - `processBlock()`: Read from tape loop to output

- **`OutputBus.h`**
  - Output routing and mixing
  - Per-track output destination selection

#### `/Source/Frontends/`
User interface implementations

##### `/Source/Frontends/Basic/`
Minimal UI with essential looper controls

- **`MainComponent.h/cpp`**
  - Main window for Basic frontend
  - Contains track list and global controls
  - Implements timer for UI updates

- **`LooperTrack.h/cpp`**
  - Single track UI component
  - Record, play/pause, stop buttons
  - Speed slider, mute button
  - File load button

##### `/Source/Frontends/Text2Sound/`
Extended UI with AI text-to-sound generation

- **`MainComponent.h/cpp`**
  - Extends Basic with Gradio integration
  - Adds Gradio settings button
  - Stores Gradio URL configuration

- **`LooperTrack.h/cpp`**
  - Track UI with text prompt input
  - "Generate" button to call Gradio API
  - Shows generation status/progress

##### `/Source/Frontends/VampNet/`
UI for VampNet audio variation generation

- **`MainComponent.h/cpp`**
  - Similar to Text2Sound but for VampNet
  - VampNet-specific parameters

- **`LooperTrack.h/cpp`**
  - Track UI with VampNet controls
  - Uses existing audio as input
  - Generates variations

##### `/Source/Frontends/Shared/`
Reusable UI components

- **`WaveformDisplay.h/cpp`**
  - Visual representation of audio buffer
  - Shows playback position
  - Click to seek

- **`TransportControls.h/cpp`**
  - Standard transport buttons (record, play, stop)
  - Consistent styling across frontends

- **`ParameterKnobs.h/cpp`**
  - Rotary controls for parameters
  - Speed, volume, etc.

- **`LevelControl.h/cpp`**
  - Volume/gain slider with meters

- **`OutputSelector.h/cpp`**
  - Output bus routing selector

#### `/Source/GradioClient/`
HTTP client for Gradio API integration

- **`GradioClient.h/cpp`**
  - RESTful HTTP client for Gradio spaces
  - Handles file upload/download
  - Polling-based async request handling
  - **Key Methods:**
    - `processRequest()`: Main entry point for generation
    - `uploadFileRequest()`: Upload audio to Gradio
    - `makePostRequestForEventID()`: Initiate generation
    - `getResponseFromEventID()`: Poll for results
    - `downloadFileFromURL()`: Download generated audio

#### Root Source Files

- **`Main.cpp`**
  - Application entry point
  - `TapeLooperApplication` class (JUCE app)
  - Shows `StartupDialog` on launch
  - Creates selected frontend window
  - Handles app lifecycle

- **`StartupDialog.h/cpp`**
  - Initial configuration dialog
  - Select number of tracks
  - Choose frontend (Basic/Text2Sound/VampNet)
  - Audio device setup

- **`CustomLookAndFeel.h`**
  - Custom JUCE styling
  - Dark theme colors and fonts

---

## Data Flow

### Recording Flow

```
User clicks "Record" button
        │
        ▼
Frontend updates TrackState.writeHead.isRecording
        │
        ▼
Audio callback (real-time thread)
        │
        ▼
MultiTrackLooperEngine::audioDeviceIOCallbackWithContext()
        │
        ▼
LooperTrackEngine::processBlock()
        │
        ▼
LooperWriteHead::processBlock()
        │
        ▼
Writes audio samples to TapeLoop buffer
        │
        ▼
User clicks "Record" again to stop
        │
        ▼
LooperWriteHead finalizes, sets recordedLength
        │
        ▼
Automatically transitions to playback
```

### Playback Flow

```
Audio callback (real-time thread)
        │
        ▼
MultiTrackLooperEngine::audioDeviceIOCallbackWithContext()
        │
        ▼
LooperTrackEngine::processBlock()
        │
        ▼
LooperReadHead::processBlock()
        │
        ▼
Reads from TapeLoop buffer with interpolation
        │
        ▼
Applies playback speed
        │
        ▼
Routes through OutputBus
        │
        ▼
Mixes to output channels
```

### AI Generation Flow (Text2Sound/VampNet)

```
User enters text prompt / selects audio
        │
        ▼
User clicks "Generate"
        │
        ▼
Frontend spawns background thread
        │
        ▼
GradioClient::processRequest()
        │
        ├─► Upload audio file (if needed)
        │
        ├─► POST request with parameters
        │
        ├─► Get event ID
        │
        ├─► Poll for completion
        │
        ├─► Download generated audio
        │
        ▼
LooperTrackEngine::loadFromFile()
        │
        ▼
Audio loaded into TapeLoop buffer
        │
        ▼
UI updates, user can play generated audio
```

---

## Key Design Patterns

### 1. Real-Time Audio Thread Safety
- **Pattern**: Lock-free atomic operations
- **Implementation**: 
  - All `TrackState` members that cross thread boundaries are `std::atomic`
  - `TapeLoop` uses `juce::CriticalSection` only for buffer allocation
  - No locks in audio callback path

### 2. State Management
- **Pattern**: Centralized state objects
- **Implementation**:
  - `TrackState` struct contains all per-track data
  - UI reads state in timer callback
  - UI writes control parameters (speed, mute, etc.)
  - Engine writes playback state (position, isPlaying)

### 3. Frontend Abstraction
- **Pattern**: Namespace-based polymorphism
- **Implementation**:
  - Each frontend in separate namespace
  - All implement `MainComponent` and `LooperTrack`
  - Main.cpp switches at runtime based on user selection
  - Shared components in `Frontends/Shared/`

### 4. Dependency Injection
- **Pattern**: Constructor injection
- **Implementation**:
  - `MainWindow` receives frontend choice and device setup
  - Frontends receive track count
  - GradioClient receives space info

---

## Threading Model

### Threads in the Application

1. **Main/Message Thread**
   - UI rendering and event handling
   - Button clicks, slider changes
   - Timer callbacks for UI updates
   - File I/O operations

2. **Audio Thread** (real-time priority)
   - `audioDeviceIOCallbackWithContext()` callback
   - Must be lock-free and deterministic
   - Processes all tracks in sequence
   - Cannot allocate memory

3. **Background Worker Threads**
   - Gradio HTTP requests
   - File loading operations
   - Spawned via `juce::Thread` or lambdas

### Thread Communication

- **Audio → UI**: Atomic reads in timer callback
- **UI → Audio**: Atomic writes of control parameters
- **Background → UI**: `juce::MessageManager::callAsync()`

---

## Building and Extending

### Adding a New Frontend

1. Create namespace in `/Source/Frontends/YourFrontend/`
2. Implement `MainComponent.h/cpp` with constructor taking `int numTracks`
3. Implement `LooperTrack.h/cpp` for per-track UI
4. Add to `CMakeLists.txt` target sources
5. Include in `Main.cpp` and add to frontend selection
6. Update `StartupDialog` combo box

### Adding a New AI Integration

1. Extend `GradioClient` or create new client class
2. Add methods for new API endpoints
3. Add UI controls in appropriate frontend
4. Handle async requests in background thread
5. Load results via `LooperTrackEngine::loadFromFile()`

### Adding Audio Effects

1. Create effect processor class
2. Add to `LooperReadHead::processBlock()` or create new processing stage
3. Add UI controls in shared components
4. Store parameters in `TrackState`

---

## Code Style and Conventions

### Naming Conventions
- **Classes**: PascalCase (`MultiTrackLooperEngine`)
- **Methods**: camelCase (`processBlock()`)
- **Members**: camelCase with 'member' prefix optional (`trackEngines`)
- **Constants**: camelCase with 'const' qualifier
- **Atomics**: Clear naming indicating shared state

### File Organization
- One class per .h/.cpp pair
- Header includes in order: system, JUCE, local
- Forward declarations preferred over includes when possible

### JUCE Conventions
- Use `juce::` namespace prefix explicitly
- Inherit from JUCE base classes appropriately
- Use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro

---

## Dependencies

### JUCE Modules Used
- `juce_core`: Foundation (strings, files, threads)
- `juce_audio_basics`: Audio buffers and utilities
- `juce_audio_devices`: Audio I/O and device management
- `juce_audio_formats`: Audio file reading/writing
- `juce_audio_processors`: Plugin infrastructure (minimal use)
- `juce_audio_utils`: High-level audio components
- `juce_events`: Message loop and timers
- `juce_graphics`: 2D graphics rendering
- `juce_gui_basics`: Standard UI components
- `juce_gui_extra`: Extended UI components
- `juce_data_structures`: Data management

### External Dependencies
- CMake 3.22+
- C++17 compiler
- Platform-specific audio APIs (CoreAudio, WASAPI, ALSA)

---

## Testing

### Manual Testing Checklist
- [ ] Record on single track, verify playback
- [ ] Record on multiple tracks simultaneously
- [ ] Sync tracks after different start times
- [ ] Change playback speed during playback
- [ ] Load audio file into track
- [ ] Switch audio devices
- [ ] Generate audio via Text2Sound
- [ ] Generate variation via VampNet
- [ ] Mute/unmute during playback
- [ ] Stop and restart playback

### Known Limitations
- Maximum buffer duration: 10 seconds per track
- 8 tracks maximum (configurable at compile time)
- No undo/redo functionality
- No project save/load
- Mono audio only (stereo planned)

---

## Performance Considerations

### Real-Time Audio
- All processing must complete within buffer size time
- Target: < 5ms latency at 512 sample buffer
- No memory allocation in audio thread
- Minimal branching in hot paths

### Memory Usage
- Each track buffer: `sampleRate * maxDuration * sizeof(float)`
- 8 tracks at 48kHz, 10s = ~3.84 MB
- Waveform display caching

### CPU Optimization
- Variable-speed playback uses linear interpolation (fast)
- Consider sinc interpolation for higher quality (slower)
- SIMD opportunities in mixing and effects

---

## Future Roadmap

### Planned Features
- [ ] Stereo/multichannel support
- [ ] Audio effects (reverb, delay, filters)
- [ ] MIDI synchronization
- [ ] Project save/load
- [ ] Undo/redo
- [ ] Export mixed output
- [ ] VST3 plugin version
- [ ] Improved waveform visualization
- [ ] Real-time input monitoring
- [ ] Overdub mode

### Technical Debt
- Refactor atomic state management into cleaner abstraction
- Improve error handling in GradioClient
- Add unit tests for engine components
- Document thread safety guarantees more explicitly
- Consider moving UI state out of engine layer

---

## Troubleshooting

### Audio Issues
- **No sound**: Check audio device selection in settings
- **Crackling**: Increase buffer size in audio settings
- **Latency**: Decrease buffer size, use ASIO (Windows) or CoreAudio (macOS)

### Build Issues
- **JUCE not found**: Ensure JUCE submodule is initialized
- **Compiler errors**: Verify C++17 support
- **Linking errors**: Check CMakeLists.txt module list

### Runtime Issues
- **Gradio timeout**: Check network connection, increase timeout
- **File load failure**: Verify audio format supported (WAV, AIFF, MP3, FLAC)
- **Crash on startup**: Check audio device availability

---

## Resources

### JUCE Documentation
- [JUCE Tutorials](https://juce.com/learn/tutorials)
- [JUCE API Reference](https://docs.juce.com/)
- [JUCE Forum](https://forum.juce.com/)

### Audio Programming
- [The Audio Programming Book](http://www.audiosynth.com/)
- [Designing Audio Effect Plugins in C++](https://www.willpirkle.com/)

### Project-Specific
- See `README.md` for user documentation
- See `CMakeLists.txt` for build configuration
- See inline code comments for implementation details

---

## Contributing

When contributing to this project:

1. Follow existing code style
2. Maintain thread safety in audio code
3. Test on multiple platforms if possible
4. Update this documentation for architectural changes
5. Comment non-obvious design decisions
6. Keep commits focused and atomic

---

## License

See `JUCE/LICENSE.md` for JUCE framework licensing (GPL/Commercial).
Project-specific code license TBD.

