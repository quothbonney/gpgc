using Plots
println("Hello world!")
data = []
vectors = []
max_size = 0
filename = "../test.gpgc.log"
sz = 512
open(filename) do file
    for line in eachline(file)
        words = split(line)

        if length(words) < 4
            continue
        end
        
        global max_size
        max_size = sz
        
        size = parse(Int, words[4])
        exp = max_size / size
        
        push!(data, log2(exp))
        push!(vectors, (parse(Float64, words[1]), parse(Float64, words[2]), parse(Float64, words[3]), size))
    end
end

function create_offsets(sizes)
  x0 = [0.0]
  y0 = [0.0]
  b = [0.0]

  index = 1
  while index <= length(sizes)
      while b[index] < sizes[index]
          b[index] += 1
          for i in 1:3
              insert!(b, index+1, b[index])
          end

          insert!(x0, index+1, x0[index] + 1/(2^b[index]))
          insert!(x0, index+2, x0[index])
          insert!(x0, index+3, x0[index] + 1/(2^b[index]))

          insert!(y0, index+1, y0[index])
          insert!(y0, index+2, y0[index] + 1/(2^b[index]))
          insert!(y0, index+3, y0[index] + 1/(2^b[index]))
      end

      index += 1
  end

  return x0, y0
end

decompressed = zeros(Int, sz, sz)


x0, y0 = create_offsets(data)

println(length(x0), " ", length(y0), " ", length(vectors))
for elem in 1:length(x0)
    x_o = Int(x0[elem] * sz)
    y_o = Int(y0[elem] * sz)

    if length(vectors[elem]) < 4
        println(vectors[elem])
    end

    i, j, k, size = vectors[elem]
    for m in 0:size-1
        for n in 0:size-1
            altitude = Int(round(i*m + j*n + k))
            decompressed[y_o+m+1, x_o+n+1] = altitude
        end
    end
end

println("Hello world!")

plt = heatmap(decompressed, size=(800, 800), aspect_ratio=:equal)

display(plt)

sleep(100)
savefig("output.png")
