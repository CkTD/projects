from hashlib import md5

class ConsistentHashRing:
    DEFAULT_CHORD_CAPACITY = 1 << 32
    DEFAULT_REPLICAS = 3

    def __init__(self, chord_capacity=DEFAULT_CHORD_CAPACITY, replicas=DEFAULT_REPLICAS):
        self.chord_capacity = chord_capacity
        self.replicas = replicas  # virtural node replicas of a physic node 

        self._keys = []           # list of virtual node id
        self._nodes = {}          # node id --> machine object

    def _do_hash(self, obj):
        """ return the hash value , given a string """
        return int(md5(obj.encode()).hexdigest(), 16) % self.chord_capacity

    def _iter_replicas(self, name):
        """ return an iterable of replica hashes， given a node name """
        return (self._do_hash("%s:%s"%(name, i))
                for i in range(self.replicas))
    
    def _find_pos(self, v):
        """ return the firt key in self._keys that greater than v,
        if v greater than the greatest, then return the lowest value.
        """
        if not self._keys:
            raise ValueError("locate in empty ring!")

        for i in self._keys:
            if i > v:
                return i
        return self._keys[0]
    
    def register_node(self, node):
        for key in self._iter_replicas(node.name):
            if key in self._keys:
                raise ValueError("node key %s already in ring" % key)
            self._keys.append(key)
            self._keys.sort()
            self._nodes[key] = node
            node.add_key(key)

            # have only one key...
            if len(self._keys) == 1:
                continue
            # move resource needed...
            next_key = self._find_pos(key)
            next_node = self._nodes[next_key]            
            for rsc_key, rsc in list(next_node.resources[next_key].items()):
                if self._find_pos(rsc_key) == key:
                    next_node.unregister_resource(rsc, next_key)
                    node.register_resource(rsc, key)

    def unregister_node(self, node):
        for key in self._iter_replicas(node.name):
            self._keys.remove(key) # raise KeyError for unregister not exist node
            del self._nodes[key]
            
            for rsc_key , rsc in list(node.resources[key].items()):
                to_node_key = self._find_pos(rsc_key)
                to_node = self._nodes[to_node_key]
                to_node.register_resource(rsc, to_node_key)

    def register_resource(self, resource):
        if not self._keys:
            raise Exception("Register resource to empty ring!")

        res_key = self._do_hash(resource.name)
        resource.set_key(res_key)
        node_key = self._find_pos(res_key)
        node = self._nodes[node_key]
        node.register_resource(resource, node_key)

    def unregister_resource(self, resource):
        rsc_key = self._do_hash(resource.name)
        node_key = self._find_pos(rsc_key)
        node = self._nodes[node_key]
        node.unregister_resource(resource, node_key)

    def __getitem__(self, name):
        """ given a name return the containint node key in ring """
        key = self._do_hash(name)
        return self._find_pos(key)
     
    def __str__(self):
        s = ""
        for key in self._keys:
            node = self._nodes[key]
            s += node.__str__(key)
            s += "virt_node_%d  (%s)\n" % (key, node.name)
        return s

    def plot(self):
        MCN = 0
        RSC = 1
        all = []
        node_identities = []
        for node_id in self._keys:
            node = self._nodes[node_id]
            all.append((node_id, MCN, node.name ,node.name + "(%d)"% node_id ))
            if node.name not in node_identities:
                node_identities.append(node.name)

            for k,v in node.resources[node_id].items():
                all.append((k,RSC, node.name, v.name + "(%s) in (%s - %d)" % (v.key, node.name, node_id)))
        all.sort(key= lambda x: x[0])
        
        from math import pi, cos, sin
        import matplotlib.pyplot as plt
        from matplotlib.patches import Circle, RegularPolygon

        def ca(degree):
            theta =  degree*pi/180 
            return cos(theta), sin(theta)
        def get_cmap(n, name='hsv'):
            '''Returns a function that maps each index in 0, 1, ..., n-1 to a distinct 
            RGB color; the keyword argument name must be a standard mpl colormap name.'''
            return plt.cm.get_cmap(name, n)

        cmap = get_cmap(len(node_identities))
        #cmap = [cmap(i) for i in range(len(node_identities))]  ??????????????
        cmap = ['r', 'g', 'b', 'y'] * 100 

        fig = plt.figure()
        ax = fig.add_subplot("111",aspect='equal')
        R = 5
        ax.set_xlim(-(R+3),R+3)
        ax.set_ylim(-(R+1),R+1)
        c = Circle((0,0),R,color='b', fill=False)
        ax.add_artist(c)
        ax.annotate(str(self.chord_capacity-1), xy=(R,-0.3) ,xytext=(R+1,-0.3), arrowprops=dict(
                  facecolor='b',shrink=0.00, width=1, headwidth=4, alpha=0.5))
        ax.annotate("0", xy=(R,0), xytext=(R+1,0), arrowprops=dict(
                  facecolor='b', shrink=0.00, width=1, headwidth=4, alpha=0.5))

        for id, TYPE, node_name, detail in all:
            scale = ca(id/self.chord_capacity*360)
            center = [ i*R for i in scale ]
            xy = [ i*(R+0.2) for i in scale ]
            xytext = [ i*R*1.2 for i in scale]
            color = cmap[node_identities.index(node_name)]
            if TYPE == MCN:
                draw = Circle(center,0.2, color=color)
            else:
                draw = RegularPolygon(center,5,0.2, color= color)

            ax.add_artist(draw)
            ax.annotate(detail, xy=xy, xytext=xytext, arrowprops=dict(
                  facecolor='b', shrink=0.00, width=1, headwidth=4, alpha=0.5))
        plt.show()

class Node:
    """ physic machine """
    def __init__(self, name):
        self.name = name
        self.resources = {}
    
    def add_key(self, key):
        self.resources[key] = {}

    def __str__(self, key):
        s = ""
        for resource_id, resource in self.resources[key].items():
            s += "     rsc_%d  (%s) in (%s)\n" % (resource.id, resource.name, self.name)
        return s

    def register_resource(self, resource, key):
        if resource.key in self.resources[key]:
            raise ValueError('resource %d already in virtual node %s'%(resource.key, key))
        self.resources[key][resource.key] = resource

    def unregister_resource(self, resource, key):
        del self.resources[key][resource.key] # raise KeyError for nonexistent source id

class Resource:
    def __init__(self, name):
        self.name = name    # resource name
        self.key = None     # resource key

    def set_key(self, key):
        self.key = key







if __name__ == '__main__':
    chr = ConsistentHashRing()

#######
    # for i in range(100):
    #     chr.register_node(Node("Node:%d" % i))
    # chr.plot()
    # for i in range(1000):
    #     chr.register_resource(Resource("Resource:%d" % i))
    # chr.plot()
#######

    m1 = Node("MCN1")
    m2 = Node("MCN2")
    r1 = Resource("RES1")
    r2 = Resource("RES2")
    r3 = Resource("RES3")

    chr.register_node(m1)
    chr.plot()

    # print(chr[r1.name])
    # print(chr[r2.name])
    # print(chr[r3.name])
    
    chr.register_resource(r1)
    chr.plot()
    
    chr.register_resource(r2)
    chr.plot()
    
    chr.register_resource(r3)
    chr.plot()

    chr.register_node(m2)
    chr.plot()

    chr.unregister_node(m1)
    chr.plot()

    chr.unregister_resource(r1)
    chr.plot()

    del m1
    del m2
    del r1
    del r2
    del r3
    del chr