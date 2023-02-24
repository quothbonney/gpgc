using Images, Plots, Colors, Statistics, ImageFiltering


img = load("/home/quothbonney/gpgc/test.png")
gray = Gray.(img)

gauss5 = imfilter(gray, Kernel.gaussian(5))
gauss3 = imfilter(gray, Kernel.gaussian(3))
gauss1 = imfilter(gray, Kernel.gaussian(1))


function std_analyze()
    K_SIZE = 5
    h = 512; w=512
    decon = zeros(512, 512)
    for i = K_SIZE+1:h-K_SIZE
        for j = K_SIZE+1:w-K_SIZE
            arr = Float32.(gray[i-K_SIZE:5:i+K_SIZE, j-K_SIZE:5:j+K_SIZE])
            decon[i, j] = (std(arr) ^ 0.5) * 10
        end
    end
    return decon
end

function post(decon)
    arr = zeros(512, 512)
    for i in CartesianIndices(decon)
        if decon[i] < 2
          val = decon[i] / 2
          antival = 1 - val
          z = (antival * gauss3[i]) + (val * gray[i])
          arr[i] = z
        elseif decon[i] < 4
          arr[i] = gauss1[i]
        end
    end
    return arr
end


println("Hello")
decon = std_analyze()
arr = post(decon)
heatmap(arr)