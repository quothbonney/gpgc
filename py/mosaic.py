import matplotlib.pyplot as plt
import numpy as np
import random

iterations = []

def mosaic(it_size, raster_size, offX, offY):
    if offX >= raster_size - 1 or offY >= raster_size - 1:
        return

    if (offX + it_size == raster_size) or (offY + it_size == raster_size):
        iterations.append((offX, offY, it_size))
        return

    elif (offX + it_size < raster_size) and (offY + it_size < raster_size):
        iterations.append((int(offX), int(offY), int(it_size)))

        new_size = it_size
        mosaic(new_size, raster_size, offX, offY + it_size)
        mosaic(new_size, raster_size, offX + it_size, offY)
        mosaic(new_size, raster_size, offX + it_size, offY + it_size)

    else:
        new_size = int(it_size / 2)
        mosaic(new_size, raster_size, offX, offY)
        mosaic(new_size, raster_size, offX, offY + it_size)
        mosaic(new_size, raster_size, offX + it_size, offY)
        mosaic(new_size, raster_size, offX + it_size, offY + it_size)
    return

if __name__ == '__main__':
    sz = 1000
    mosaic(512, sz, 0, 0)
    mos = np.zeros((sz, sz), dtype=int)
    print(iterations)
    for fragment in iterations:
        color = random.randint(0, 255)
        for i in range(fragment[2]):
            for j in range(fragment[2]):
                mos[i + fragment[0]][j + fragment[1]] = color

    plt.imshow(mos)
    plt.show()


