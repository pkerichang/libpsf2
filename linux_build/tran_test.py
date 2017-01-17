from __future__ import print_function, division

import h5py
import matplotlib.pyplot as plt

fname = 'test.hdf5'
key_name = 'time'

f = h5py.File(fname, 'r')

out_list = []
for k in f.keys():
    dset = f[k]
    if k != key_name:
        out_list.append((k, dset[()]))
    print(dset)
    print('dest %s attrs:' % k)
    for m in f[k].attrs:
        print('%s = %s' % (m, dset.attrs[m]))
    print()
print('file attrss: ')
for k in f.attrs:
    print('%s = %s' % (k, f.attrs[k]))

xvec = f[key_name][()]

plt.figure(1)
for name, out in out_list:
    if (xvec[-1] / xvec[xvec.size // 2]) > 10:
        plt.semilogx(xvec, out, label=name)
    else:
        plt.plot(xvec, out, label=name)
plt.legend()
plt.show()
