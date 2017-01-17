from __future__ import print_function

import h5py

f = h5py.File('test.hdf5', 'r')
for k in f.keys():
    dset = f[k]
    print(dset)
    print(dset[()])
    print('dest %s attrs:' % k)
    for m in f[k].attrs:
        print('%s = %s' % (m, dset.attrs[m]))
    print()
    
print('file attrss: ')
for k in f.attrs:
    print('%s = %s' % (k, f.attrs[k]))
