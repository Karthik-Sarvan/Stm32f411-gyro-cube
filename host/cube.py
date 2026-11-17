import argparse
import threading
import time

import pygame
import serial
from OpenGL.GL import (
    GL_COLOR_BUFFER_BIT,
    GL_DEPTH_BUFFER_BIT,
    GL_DEPTH_TEST,
    GL_QUADS,
    glBegin,
    glClear,
    glColor3fv,
    glEnable,
    glEnd,
    glPopMatrix,
    glPushMatrix,
    glRotatef,
    glTranslatef,
    glVertex3fv,
)
from OpenGL.GLU import gluPerspective

WINDOW_SIZE = (800, 600)
angles = [0.0, 0.0, 0.0]


def read_serial(port):
    global angles
    while True:
        try:
            with serial.Serial(port, 115200, timeout=1) as device:
                while True:
                    fields = device.readline().decode(errors="ignore").strip().split(",")
                    if len(fields) == 4 and fields[0] == "GYRO":
                        try:
                            angles = [float(value) for value in fields[1:]]
                        except ValueError:
                            continue
        except serial.SerialException as error:
            print(f"Serial connection lost; retrying: {error}")
            time.sleep(1)


def draw_cube():
    vertices = (
        (-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
        (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1),
    )
    faces = (
        (0, 1, 2, 3), (4, 5, 6, 7), (0, 4, 5, 1),
        (2, 6, 7, 3), (1, 5, 6, 2), (4, 0, 3, 7),
    )
    colors = (
        (1, 0, 0), (0, 1, 0), (0, 0, 1),
        (1, 1, 0), (1, 0, 1), (0, 1, 1),
    )
    glBegin(GL_QUADS)
    for face, color in zip(faces, colors):
        glColor3fv(color)
        for vertex in face:
            glVertex3fv(vertices[vertex])
    glEnd()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="USB CDC device, e.g. /dev/ttyACM0")
    parser.add_argument(
        "--rotation-gain",
        type=float,
        default=5.0,
        help="multiply sensor angles before drawing the cube (default: 5)",
    )
    args = parser.parse_args()
    threading.Thread(target=read_serial, args=(args.port,), daemon=True).start()
    pygame.init()
    pygame.display.set_mode(WINDOW_SIZE, pygame.OPENGL | pygame.DOUBLEBUF)
    gluPerspective(45, WINDOW_SIZE[0] / WINDOW_SIZE[1], 0.1, 50)
    glTranslatef(0, 0, -6)
    clock = pygame.time.Clock()
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glEnable(GL_DEPTH_TEST)
        glPushMatrix()
        glRotatef(angles[2] * args.rotation_gain, 0, 0, 1)
        glRotatef(angles[0] * args.rotation_gain, 0, 1, 0)
        glRotatef(angles[1] * args.rotation_gain, 1, 0, 0)
        draw_cube()
        glPopMatrix()
        pygame.display.flip()
        clock.tick(60)
    pygame.quit()


if __name__ == "__main__":
    main()
