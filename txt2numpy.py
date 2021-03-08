import numpy as np 

data = np.loadtxt("isosurface_cameras.txt")
print(data.shape)

new = []

for i, d in enumerate(data):
    new.append(d[0:6])

new = np.array(new)

with open("view.npy", 'wb') as f:
    np.save(f, new)
    
new = np.load("view.npy")
print(new.shape)