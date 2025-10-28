from __future__ import (division, print_function, absolute_import)

from ._data import Data

import copy
import json
import numpy as np
import base64


class VideoEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, np.integer):
            return int(obj)
        elif isinstance(obj, np.floating):
            return float(obj)
        if isinstance(obj, np.ndarray):
            data = obj
            obj = {
                '_type': 'numpy.ndarray',
                'size': data.size,
                'shape': data.shape,
                'data': {
                    '_encoding': 'base64',
                    '_dtype': data.dtype.name,
                    'value': base64.urlsafe_b64encode(data.tostring()).decode()
                },
            }
            return obj
        return super().default(obj)


class Frame:

    def __init__(self, video, tree):
        self._tree = tree

        for name in tree._imported_attrs:
            setattr(self, name, getattr(tree, name))

        for attr in ('k', 'r', 'g', 'b', 'raw'):
            if hasattr(self, attr):
                setattr(self, attr, getattr(self, attr).reshape((video.height, video.width)))


class Video(Data):

    def __init__(self, tree):
        self._tree = tree

        data = self._tree['data']
        for name in data._imported_attrs:
            setattr(self, name, getattr(data, name))

        frames = self._tree['data/frames']
        if isinstance(frames, list):
            self.frames = []
            for frame in frames:
                self.frames.append(Frame(self, frame))
        else:
            self.frames = [Frame(self, frames)]

    def plot(self):
        import matplotlib.pyplot as plt
        import matplotlib.animation as animation
        fig = plt.figure()

        ims = []
        for frame in self.frames:
            if self.is_color:
                img = np.zeros((self.height, self.width, 3), dtype='uint8')
                img[:,:,0] = frame.r.reshape(img.shape[:2])
                img[:,:,1] = frame.g.reshape(img.shape[:2])
                img[:,:,2] = frame.b.reshape(img.shape[:2])
                ims.append([plt.imshow(img, animated=True, interpolation='bilinear', resample=False)])
            else:
                img = frame.k.reshape((self.height, self.width))
                ims.append([plt.imshow(img, cmap=plt.cm.gray, animated=True, interpolation='bilinear', resample=False)])

        ani = animation.ArtistAnimation(fig, ims)
        plt.show()

    def widget(self):
        raise NotImplementedError("widget function not implemented for Video objects")

    def jsonify(self, indent=None):
        raise NotImplementedError("jsonify has not been implement for Video objects")

    def clone(self):
        """
        Return a deepcopy of a video object
        """
        return copy.deepcopy(self)

