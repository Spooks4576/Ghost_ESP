import pyaudio
import numpy as np
import socket
import logging

# Replace with the IP address and port of your ESP32
UDP_IP = "192.168.1.255"
UDP_PORT = 6677

# Audio configuration
CHUNK = 1024  # Adjusted chunk size for FFT analysis
FORMAT = pyaudio.paInt16  # Sample size and format
CHANNELS = 1  # Mono audio
RATE = 44100  # Sampling rate in Hz

# Scaling factor to adjust the amplitude range
AMPLITUDE_SCALING_FACTOR = 2

# Gravity rate for amplitude decay
GRAVITY = 0.95  # Adjust this value to control the rate of decay (like gravity)

# Noise threshold to eliminate low-level signals
NOISE_THRESHOLD = 0.2  # Amplitude below which is considered noise and set to zero

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def send_amplitude(amplitude):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        message = f"{amplitude}".encode('utf-8')
        sock.sendto(message, (UDP_IP, UDP_PORT))
        sock.close()
        logging.debug(f"Sent amplitude: {amplitude} to {UDP_IP}:{UDP_PORT}")
    except Exception as e:
        logging.error(f"Failed to send data: {e}")

def calculate_amplitude_fft(audio_data):
    """
    Calculates amplitude using FFT method, focusing on the kick drum range.
    """
    try:
        # Apply a window function to reduce spectral leakage
        window = np.hanning(len(audio_data))
        windowed_data = audio_data * window

        # Perform FFT
        fft_data = np.fft.rfft(windowed_data)
        magnitude_spectrum = np.abs(fft_data)

        # Compute frequency bins
        freqs = np.fft.rfftfreq(len(windowed_data), d=1./RATE)

        # Focus on the kick drum range (e.g., 20 Hz to 60 Hz)
        lowcut = 20
        highcut = 60
        freq_indices = np.where((freqs >= lowcut) & (freqs <= highcut))[0]
        amplitude = np.sum(magnitude_spectrum[freq_indices])

        # Scale amplitude
        scaled_amplitude = amplitude * AMPLITUDE_SCALING_FACTOR / 1e6

        # Apply noise threshold
        if scaled_amplitude < NOISE_THRESHOLD:
            scaled_amplitude = 0.0

        # Clamp amplitude between 0.0 and 1.0
        scaled_amplitude = min(max(scaled_amplitude, 0.0), 1.0)

        return scaled_amplitude
    except Exception as e:
        logging.error(f"Error in calculate_amplitude_fft: {e}")
        return 0.0

def main():
    logging.info("Starting audio streaming and data transmission to ESP32.")
    p = pyaudio.PyAudio()

    stream = None
    amplitude = 0.0  # Initialize amplitude
    dynamic_threshold = NOISE_THRESHOLD * 1.5  # Initial threshold for beat detection
    try:
        # Get the default input device index
        default_device_index = p.get_default_input_device_info()['index']
        device_name = p.get_device_info_by_index(default_device_index)['name']
        logging.info(f"Using default audio input device: {device_name} (Index: {default_device_index})")

        # Open audio stream
        stream = p.open(format=FORMAT,
                        channels=CHANNELS,
                        rate=RATE,
                        input=True,
                        frames_per_buffer=CHUNK,
                        input_device_index=default_device_index)

        logging.info("Audio stream opened. Starting to send amplitude data to ESP32...")

        while True:
            try:
                data = stream.read(CHUNK, exception_on_overflow=False)
                logging.debug("Audio data read successfully.")

                # Convert binary data to numpy array
                audio_data = np.frombuffer(data, dtype=np.int16)

                # Calculate the amplitude using FFT method
                current_amplitude = calculate_amplitude_fft(audio_data)

                # Detect transient spikes typical of a kick
                if current_amplitude > dynamic_threshold:
                    logging.info(f"Kick detected with amplitude: {current_amplitude}")
                    amplitude = current_amplitude  # Update amplitude with detected value

                # Send the current amplitude even as it decays
                send_amplitude(amplitude)

                # Apply "gravity" to pull the amplitude back towards zero
                amplitude = max(amplitude - GRAVITY, 0.0)

                # Adjust the dynamic threshold slowly to adapt to the environment
                dynamic_threshold = max(NOISE_THRESHOLD, dynamic_threshold * 0.99 + current_amplitude * 0.01)

            except IOError as e:
                logging.warning(f"Audio input overflow or other error: {e}")
            except KeyboardInterrupt:
                logging.info("KeyboardInterrupt received. Exiting...")
                break
            except Exception as e:
                logging.error(f"Error during audio processing: {e}")

    except Exception as e:
        logging.critical(f"Failed to initialize audio stream: {e}")
    finally:
        logging.info("Terminating audio stream.")
        if stream is not None:
            stream.stop_stream()
            stream.close()
        p.terminate()
        logging.info("Audio stream closed and resources released.")

if __name__ == "__main__":
    main()
