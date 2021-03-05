import math
import os
import socket

import numpy as np
import pygame
from matplotlib import colors


def draw_display():
    on_color = [x * 255 for x in colors.to_rgb("blue")]
    off_color = [x * 255 for x in colors.to_rgb("red")]

    for i in range(num_sectors):
        sector_angle = (2 * math.pi) / num_sectors
        start_angle = i * sector_angle
        end_angle = i * sector_angle + sector_angle

        for j in range(num_pixels):
            left = canvas_center[0] - (circle_radius * (j + pixel_offset) / num_pixels)
            top = canvas_center[1] - (circle_radius * (j + pixel_offset) / num_pixels)
            radius = circle_radius * (j + pixel_offset) / num_pixels

            arc_rect = pygame.Rect(left, top, radius * 2, radius * 2)
            if pixel_state[i][j]:
                color = on_color
            else:
                color = off_color

            pygame.draw.arc(canvas, color, arc_rect, start_angle, end_angle, pixel_height)


# returns (radius, angle)
def rect_to_polar(x, y):
    x = x - canvas_center[0]
    y = -(y - canvas_center[1])
    magnitude = np.sqrt(x ** 2 + y ** 2)
    angle = np.arctan2(y, x)
    if angle < 0:
        angle = 2 * np.pi + angle

    return magnitude, angle * R2D


def polar_to_pixel(radius, angle):
    pixel = int((radius - (pixel_offset - 1) * pixel_height) * num_pixels / circle_radius)
    sector = int(angle * num_sectors / 360)
    print(pixel, sector)
    return sector, pixel


def display_to_string(display):
    file_name = 'Generated_Map.h'
    save_path = 'D:\Engineering Projects\Diabolo POV\Code\YoyoLightOTA'
    name = os.path.join(save_path, file_name)

    with open(name, 'w') as f:
        f.write(f"#define NUM_LEDS \t\t {num_pixels} \n")
        f.write(f"#define NUM_SECTORS \t {num_sectors} \n\n")

        f.write("const uint8_t GENERATED_DISPLAY")
        f.write(f"[{num_sectors}][{num_pixels}] = {{\n")
        for sector in display:
            f.write("\t{")
            for led_state in sector:
                if led_state:
                    f.write("1, ")
                else:
                    f.write("0, ")
            f.write("},\n")
        f.write("};")


def UDP_message():
    # Receive BUFFER_SIZE bytes data
    # data is a list with 2 elements
    # first is data
    # second is client address
    data = s.recvfrom(BUFFER_SIZE)
    if data:
        # print received data
        print('Client to Server: ', data)
        # Convert to upper case and send back to Client
        s.sendto(data[0].upper(), data[1])


# Close connection

if __name__ == "__main__":

    # bind all IP
    HOST = '0.0.0.0'
    # Listen on Port
    PORT = 8266
    # Size of receive buffer
    BUFFER_SIZE = 1024
    # Create a TCP/IP socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Bind the socket to the host and port
    s.bind((HOST, PORT))



    pygame.init()

    R2D = 180 / np.pi  # radians to degrees

    num_sectors = 60  # how many chunks along circumference
    num_pixels = 10  # how many "LEDs" along radius
    pixel_state = np.zeros(shape=(num_sectors, num_pixels), dtype=bool)

    canvas_width = 720
    canvas_height = 720
    canvas_center = (canvas_width / 2, canvas_height / 2)

    canvas = pygame.display.set_mode([canvas_width, canvas_height])

    circle_radius = canvas_width * .9 / 2
    pixel_height = int(circle_radius * .9 / num_pixels)
    pixel_offset = 1

    running = True
    sel_mode = False
    prev_press = False
    pressed = False
    press_button = 0
    curr_sector = 0
    mouse_pos = None

    while running:
        # UDP_message()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.MOUSEBUTTONDOWN:
                pressed = True
                press_button = event.button
            if (event.type == pygame.MOUSEMOTION) and pressed or (prev_press is not pressed):
                mouse_pos = pygame.mouse.get_pos()
                print(mouse_pos)
                sel_pixel = polar_to_pixel(*rect_to_polar(*mouse_pos))
                try:
                    if press_button == 1:
                        pixel_state[sel_pixel[0]][sel_pixel[1]] = True
                    elif press_button == 3:
                        pixel_state[sel_pixel[0]][sel_pixel[1]] = False
                except IndexError:
                    pass
            if event.type == pygame.MOUSEBUTTONUP:
                pressed = False
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    pixel_state.fill(False)
                if event.key == pygame.K_f:
                    print(pixel_state)
                if event.key == pygame.K_p:
                    display_to_string(pixel_state)
        draw_display()
        # pixel_state[curr_sector][5] = not pixel_state[curr_sector][5]
        # curr_sector = (curr_sector + 1) % num_sectors

        pygame.display.flip()

    s.close()
    pygame.quit()
