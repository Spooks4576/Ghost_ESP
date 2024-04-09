import pygame
import json
import time

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() > 0:
    joystick = pygame.joystick.Joystick(0)
    joystick.init()
else:
    print("No joystick detected.")
    exit()

joystick_mapping = {
    0: "BUTTON_A",
    1: "BUTTON_B",
    2: "BUTTON_X",
    3: "BUTTON_Y",
    4: "BUTTON_LBUMPER",
    5: "BUTTON_RBUMPER",
    # Assuming D-pad is represented as buttons:
    11: "DPAD_UP",
    12: "DPAD_DOWN",
    13: "DPAD_LEFT",
    14: "DPAD_RIGHT",
    # Additional mappings as needed
}


current_pressed_buttons = set()

action_sequence = []
last_time = time.time()


time_threshold = 100

try:
    while True:
        events = pygame.event.get()
        for event in events:
            if event.type in [pygame.JOYBUTTONDOWN, pygame.JOYBUTTONUP]:
                button = joystick_mapping.get(event.button)
                if button:
                    if event.type == pygame.JOYBUTTONDOWN:
                        current_pressed_buttons.add(button)
                    else:
                        current_pressed_buttons.discard(button)
        
        
        if events:
            current_time = time.time()
            delay = int((current_time - last_time) * 1000)
            
            
            if delay < time_threshold and current_pressed_buttons:
                action = {
                    "type": "nsw",
                    "buttons": list(current_pressed_buttons),
                    "action": "press",
                    "delay": delay
                }
                action_sequence.append(action)
                print(f"Detected: {action}")
            
            last_time = current_time

except KeyboardInterrupt:
    with open('action_sequence.json', 'w') as file:
        json.dump({"sequence": action_sequence}, file, indent=4)
    print("\nAction sequence saved to 'action_sequence.json'.")