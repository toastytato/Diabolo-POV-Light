import math

import numpy as np
import pygame
from matplotlib import colors


def draw_display():

    on_color = [x * 255 for x in colors.to_rgb("blue")]
    off_color = [x * 255 for x in colors.to_rgb("red")]

    for i in range(num_sectors):
        sector_angle = (2 * math.pi) / num_sectors
        start_angle = i * sector_angle
        end_angle = i * sector_angle + sector_angle * 1

        for j in range(num_pixels):
            left = canvas_center[0] - (circle_radius * (j + center_offset) / num_pixels)
            top = canvas_center[1] - (circle_radius * (j + center_offset) / num_pixels)
            radius = circle_radius * (j + center_offset) / num_pixels

            arc_rect = pygame.Rect(left, top, radius * 2, radius * 2)
            if pixel_state[i][j]:
                color = on_color
            else:
                color = off_color

            pygame.draw.arc(canvas, color, arc_rect, start_angle, end_angle, width)


# returns (radius, angle)
def rect_to_polar(x, y):
    x = x - canvas_center[0]
    y = -(y - canvas_center[1])
    radius = np.sqrt(x ** 2 + y ** 2)
    angle = np.arctan2(y, x)
    if angle < 0:
        angle = 2 * np.pi + angle

    return radius, angle * rad_to_deg

def polar_to_pixel(radius, angle):
    pixel = int((radius - width) * num_pixels / circle_radius)
    sector = int(angle * num_sectors / 360)
    print(pixel, sector)
    return sector, pixel


if __name__ == "__main__":

    pygame.init()

    rad_to_deg = 180/np.pi

    num_sectors = 30  # how many chunks along circumference
    num_pixels = 10  # how many "LEDs" along radius
    pixel_state = np.zeros(shape=(num_sectors, num_pixels), dtype=bool)

    canvas_width = 720
    canvas_height = 720
    canvas_center = (canvas_width / 2, canvas_height / 2)

    canvas = pygame.display.set_mode([canvas_width, canvas_height])

    circle_radius = canvas_width * .8 / 2
    width = int(circle_radius * .8 / num_pixels)
    center_offset = 2

    running = True

    curr_sector = 0

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                print(pos)
                sel_pixel = polar_to_pixel(*rect_to_polar(*pos))
                try:
                    pixel_state[sel_pixel[0]][sel_pixel[1]] = not pixel_state[sel_pixel[0]][sel_pixel[1]]
                except IndexError:
                    pass
        draw_display()
        # pixel_state[curr_sector][5] = not pixel_state[curr_sector][5]
        # curr_sector = (curr_sector + 1) % num_sectors

        pygame.display.flip()

    pygame.quit()
