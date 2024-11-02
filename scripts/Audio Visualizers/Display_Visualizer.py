import pyaudio
import numpy as np
import socket
import time

# Audio settings
CHUNK = 1024             # Number of audio samples per frame
RATE = 44100             # Sampling rate in Hz
FORMAT = pyaudio.paInt16 # Audio format
CHANNELS = 1             # Mono audio

# Network settings
UDP_PORT = 6677          # Port number
BROADCAST_IP = '192.168.1.255'  # Broadcast address

# Track info
TRACK_NAME = input("Enter Track Name ")
ARTIST_NAME = input("Enter Artist Name ")

# Initialize PyAudio and UDP socket
p = pyaudio.PyAudio()
stream = p.open(format=FORMAT, channels=CHANNELS, rate=RATE, input=True, frames_per_buffer=CHUNK)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)  # Enable broadcasting

# Frequency bands
num_bands = 15  # Number of amplitude values you want
NOISE_THRESHOLD = 100  # Lowered for better sensitivity to quieter sounds

# Define manually adjusted frequency bands for better sensitivity
band_edges = np.array([20, 60, 150, 300, 600, 1200, 2500, 5000, 8000, 12000, 16000, 20000, 24000, 28000, 32000, RATE / 2])

# Prepare frequency bins
freq_bins = np.fft.rfftfreq(CHUNK, d=1./RATE)

# Initialize variables for normalization, smoothing, and decay
previous_amplitudes = np.zeros(num_bands)
decay_factor = 0.9  # Adjust between 0 (no decay) and 1 (fast decay)
decay_strength = 1  # Strength of decay, adjust for slower or faster fade

try:
    while True:
        data = stream.read(CHUNK, exception_on_overflow=False)
        audio_data = np.frombuffer(data, dtype=np.int16)

        # Apply window function to reduce spectral leakage
        window = np.hanning(len(audio_data))
        filtered_audio_data = audio_data * window

        # Perform FFT
        fft_data = np.fft.rfft(filtered_audio_data)
        fft_magnitude = np.abs(fft_data)

        # Calculate amplitude for each band
        band_amplitudes = []
        for i in range(num_bands):
            idx = np.where((freq_bins >= band_edges[i]) & (freq_bins < band_edges[i+1]))[0]
            if len(idx) > 0:  # Ensure the band contains frequency bins
                amplitude = np.mean(fft_magnitude[idx])
            else:
                amplitude = 0

            # Apply noise threshold
            if amplitude < NOISE_THRESHOLD:
                amplitude = 0
            band_amplitudes.append(amplitude)

        # Normalize amplitudes
        max_amplitude = max(band_amplitudes)
        if max_amplitude == 0:
            normalized_amplitudes = [0] * num_bands
        else:
            # Normalize and ensure no NaN values
            normalized_amplitudes = [int(np.nan_to_num((amp / max_amplitude) * 200)) for amp in band_amplitudes]

        # Apply decay to each bar for a smoother fall-off effect
        for i in range(num_bands):
            if normalized_amplitudes[i] < previous_amplitudes[i]:
                # Gradually reduce the amplitude towards zero using decay factor
                previous_amplitudes[i] = max(previous_amplitudes[i] * decay_factor, normalized_amplitudes[i])
            else:
                # Update to the new higher amplitude
                previous_amplitudes[i] = normalized_amplitudes[i]

            # Apply additional decay to smooth the effect
            previous_amplitudes[i] -= decay_strength
            # Ensure it doesn't go below zero
            previous_amplitudes[i] = max(0, previous_amplitudes[i])

        # Convert previous amplitudes to integer for sending
        final_amplitudes = [int(amp) for amp in previous_amplitudes]

        # Create a message including the track info and normalized amplitudes
        message = TRACK_NAME.encode('utf-8')[:32].ljust(32, b'\0')  # 32 bytes for track name
        message += ARTIST_NAME.encode('utf-8')[:32].ljust(32, b'\0')  # 32 bytes for artist name
        message += bytes(final_amplitudes)  # Include all 15 amplitude values

        # Ensure the message length is 79 bytes
        if len(message) != 79:
            print(f"Message length is {len(message)}, expected 79 bytes")

        # Send the message over UDP to the broadcast address
        sock.sendto(message, (BROADCAST_IP, UDP_PORT))
except KeyboardInterrupt:
    pass
finally:
    stream.stop_stream()
    stream.close()
    p.terminate()
    sock.close()