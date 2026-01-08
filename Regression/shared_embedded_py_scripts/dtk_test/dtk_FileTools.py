#!/usr/bin/python

"""
Support for three formats of serialized population files:
1. "Original version": single payload chunk with simulation and all nodes, uncompressed or snappy or LZ4
2. "First chunked version": multiple payload chunks, one for simulation and one each for nodes
3. "Second chunked version": multiple payload chunks, simulation and node objects are "root" objects in each chunk
4. "Metadata update": compressed: true|false + engine: NONE|LZ4|SNAPPY replaced with compression: NONE|LZ4|SNAPPY
"""

import copy
import gc
from collections.abc import MutableMapping
import json
import os
import time
import dtk_test.dtk_FileSupport as support
#import emod_api.serialization.dtkFileSupport as support

IDTK = 'IDTK'
MAX_VERSION = 6


# -----------------------------------------------------------------------------
# --- compression helpers
# -----------------------------------------------------------------------------
NONE = 'NONE'
LZ4 = 'LZ4'
SNAPPY = 'SNAPPY'

__engines__ = {LZ4: support.EllZeeFour, SNAPPY: support.Snappy, NONE: support.Uncompressed}

# V6 compression strings are fixed to always be three characters so that
# the header size is predictable regardless of compression type used.
V6_COMPRESSION_STR_NONE = "NON"
V6_COMPRESSION_STR_LZ4 = "LZ4"
V6_COMPRESSION_STR_SNAPPY = "SNA"


def _determine_v6_compression_type(data):
    if len(data) < 0x7E000000:
        return V6_COMPRESSION_STR_LZ4
    elif len(data) < 0xFFFFFFFF:
        return V6_COMPRESSION_STR_SNAPPY
    else:
        return V6_COMPRESSION_STR_NONE


def _compression_type_v6_to_old(compression_str):
    if compression_str == V6_COMPRESSION_STR_NONE:
        return NONE
    elif compression_str == V6_COMPRESSION_STR_LZ4:
        return LZ4
    elif compression_str == V6_COMPRESSION_STR_SNAPPY:
        return SNAPPY
    else:
        raise RuntimeError(f"Unknown/unsupported compression scheme '{compression_str}'")


def _compression_type_old_to_v6(compression_str):
    if compression_str == NONE:
        return V6_COMPRESSION_STR_NONE
    elif compression_str == LZ4:
        return V6_COMPRESSION_STR_LZ4
    elif compression_str == SNAPPY:
        return V6_COMPRESSION_STR_SNAPPY
    else:
        raise RuntimeError(f"Unknown/unsupported compression scheme '{compression_str}'")


def uncompress(data, engine):
    if engine in __engines__:
        return __engines__[engine].uncompress(data)
    else:
        raise RuntimeError(f"Unknown/unsupported compression scheme '{engine}'")


def compress(data, engine):
    if engine in __engines__:
        return __engines__[engine].compress(data)
    else:
        raise RuntimeError(f"Unknown/unsupported compression scheme '{engine}'")


# -----------------------------------------------------------------------------
# --- DtkHeader
# -----------------------------------------------------------------------------

class DtkHeader(support.SerialObject):
    # noinspection PyDefaultArgument
    def __init__(self, dictionary=None):
        if dictionary is None:
            dictionary = {
                'author': 'unknown',
                'bytecount': 0,
                'chunkcount': 0,
                'chunksizes': [],
                'compressed': True,
                'date': time.strftime('%a %b %d %H:%M:%S %Y'),
                'engine': LZ4,
                'tool': os.path.basename(__file__),
                'version': 1}
        super(DtkHeader, self).__init__(dictionary)
        return

    def __str__(self):
        text = json.dumps(self, separators=(',', ':'))
        return text

    def __len__(self):
        length = len(self.__str__())
        return length

# -----------------------------------------------------------------------------
# --- DtkFile
# -----------------------------------------------------------------------------

class DtkFile(object):

    class Contents(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            data = str(uncompress(self.__parent__.chunks[index], self.__parent__.compression), 'utf-8')
            return data

        def __setitem__(self, index, value):
            data = compress(value.encode(), self.__parent__.compression)
            self.__parent__.chunks[index] = data
            return

        def append(self, item):
            data = compress(item, self.__parent__.compression)
            self.__parent__.chunks.append(data)

        def __len__(self):
            length = len(self.__parent__.chunks)
            return length

    class Objects(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            try:
                contents = self.__parent__.contents[index]
                item = json.loads(contents, object_hook=support.SerialObject)
            except Exception:
                raise UserWarning("Could not parse JSON in chunk {0}".format(index))
            return item

        def __setitem__(self, index, value):
            contents = json.dumps(value, separators=(',', ':'))
            self.__parent__.contents[index] = contents
            return

        def append(self, item):
            contents = json.dumps(item, separators=(',', ':'))
            self.__parent__.contents.append(contents)
            return

        def __len__(self):
            length = len(self.__parent__.chunks)
            return length

    def __init__(self, header):
        self.__header__ = header
        self._chunks = [None for index in range(header.chunkcount)]
        self.contents = self.Contents(self)
        self.objects = self.Objects(self)
        return

    @property
    def header(self):
        return self.__header__

    @property
    def compressed(self):
        is_compressed = (self.__header__.engine.upper() != NONE)
        return is_compressed

    @property
    def compression(self):
        engine = self.__header__.engine.upper()
        return engine

    @compression.setter
    def compression(self, engine):
        self.__set_compression__(engine.upper())

    @property
    def byte_count(self):
        total = sum(self.chunk_sizes)
        return total

    @property
    def chunk_count(self):
        length = len(self.chunks)
        return length

    @property
    def chunk_sizes(self):
        sizes = [len(chunk) for chunk in self.chunks]
        return sizes

    # Optional header entries
    @property
    def author(self):
        return self.__header__.author if 'author' in self.__header__ else ''

    @author.setter
    def author(self, value):
        self.__header__['author'] = str(value)
        return

    @property
    def date(self):
        return self.__header__.date if 'date' in self.__header__ else ''

    @date.setter
    def date(self, value):
        self.__header__['date'] = str(value)

    @property
    def tool(self):
        return self.__header__.tool if 'tool' in self.__header__ else ''

    @tool.setter
    def tool(self, value):
        self.__header__['tool'] = str(value)
        return

    @property
    def version(self):
        return self.__header__.version

    @property
    def chunks(self):
        return self._chunks

    @property
    def nodes(self):
        return self._nodes

    def _sync_header(self):

        self.__header__.date = time.strftime('%a %b %d %H:%M:%S %Y')
        self.__header__.chunkcount = len(self.chunks)
        self.__header__.chunksizes = [len(chunk) for chunk in self.chunks]
        self.__header__.bytecount = sum(self.__header__.chunksizes)

        return

    def __set_compression__(self, engine):
        if engine != self.compression:
            for index in range(self.chunk_count):
                chunk = compress(self.contents[index], engine)
                self._chunks[index] = chunk
            self.__header__.engine = engine
            self.__header__['compressed'] = (engine != NONE)
        return

# -----------------------------------------------------------------------------
# --- DtkFileV1 
# ---
# --- "Original version": single payload chunk with simulation and all nodes,
# --- uncompressed or snappy or LZ4
# -----------------------------------------------------------------------------

class DtkFileV1(DtkFile):

    def __init__(self, header=None, filename='', handle=None):
        if header is None:
            header = DtkHeader()
        header.version = 1
        super(DtkFileV1, self).__init__(header)
        if handle is not None:
            self.chunks[0] = handle.read(header.chunksizes[0])
            self._nodes = [entry.node for entry in self.simulation.nodes]
        return

    @property
    def simulation(self):
        return self.objects[0].simulation

    @simulation.setter
    def simulation(self, value):
        self.objects[0] = {'simulation': value}
        return

# -----------------------------------------------------------------------------
# --- DtkFileV2
# ---
# --- "First chunked version": multiple payload chunks, one for simulation and
# --- one each for nodes
# -----------------------------------------------------------------------------

class DtkFileV2(DtkFile):

    class NodesV2(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                # Version 2 looks like this {'suid':{'id':id},'node':{...}}, dereference the node here for simplicity.
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            item = self.__parent__.objects[index + 1]
            return item.node

        def __setitem__(self, index, value):
            # Version 2 actually saves the entry from simulation.nodes (C++) which is a map of suid to node.
            self.__parent__.objects[index + 1] = {'suid': {'id': value.suid.id}, 'node': value}
            return

        def __len__(self):
            length = self.__parent__.chunk_count - 1
            return length

    def __init__(self, header=None, filename='', handle=None):
        if header is None:
            header = DtkHeader()
        header.version = 2
        super(DtkFileV2, self).__init__(header)
        for index, size in enumerate(header.chunksizes):
            self.chunks[index] = handle.read(size)
            if len(self.chunks[index]) != size:
                raise UserWarning(
                    "Only read {0} bytes of {1} for chunk {2} of file '{3}'".format(len(self.chunks[index]),
                                                                                    size, index, filename))
        # Version 2 looks like this: {'simulation':{...}} so we dereference the simulation here for simplicity.
        self._nodes = self.NodesV2(self)
        return

    @property
    def simulation(self):
        sim = self.objects[0]['simulation']
        del sim['nodes']
        return sim

    @simulation.setter
    def simulation(self, value):
        sim = copy.deepcopy(value)
        sim['nodes'] = []
        self.objects[0] = {'simulation': sim}
        return

# -----------------------------------------------------------------------------
# --- DtkFileV3
# ---
# --- "Second chunked version": multiple payload chunks, simulation and
# --- node objects are "root" objects in each chunk
# -----------------------------------------------------------------------------

class DtkFileV3(DtkFile):

    class NodesV3(object):
        def __init__(self, parent):
            self.__parent__ = parent
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                yield self.__getitem__(index)
                index += 1

        def __getitem__(self, index):
            item = self.__parent__.objects[index + 1]
            return item

        def __setitem__(self, index, value):
            self.__parent__.objects[index + 1] = value
            return

        def __len__(self):
            length = self.__parent__.chunk_count - 1
            return length

    def __init__(self, header=None, filename='', handle=None):
        if header is None:
            header = DtkHeader()
        header.version = 3
        super(DtkFileV3, self).__init__(header)
        for index, size in enumerate(header.chunksizes):
            self.chunks[index] = handle.read(size)
            if len(self.chunks[index]) != size:
                raise UserWarning("Only read {0} bytes of {1} for chunk {2} of file '{3}'".format(len(self.chunks[index]), size, index, filename))
        self._nodes = self.NodesV3(self)
        return

    @property
    def simulation(self):
        # from dtk-tools
        # if len(self.objects) > 0:
        #     sim = self.objects[0]
        #     del sim['nodes']
        # else:
        #     sim = {}

        sim = self.objects[0]
        del sim['nodes']
        return sim

    @simulation.setter
    def simulation(self, value):
        sim = copy.deepcopy(value)
        sim['nodes'] = []
        # from dtk-tools
        # if len(self.objects) == 0:
        #     self.objects.append(None)
        self.objects[0] = sim
        return

# -----------------------------------------------------------------------------
# --- DtkFileV4
# ---
# --- "Metadata update": compressed: true|false + engine: NONE|LZ4|SNAPPY replaced
# --- with compression: NONE|LZ4|SNAPPY
# -----------------------------------------------------------------------------
class DtkFileV4(DtkFileV3):

    def __init__(self, header=None, filename='', handle=None):
        if header is None:
            header = DtkHeader()
        super(DtkFileV4, self).__init__(header, filename, handle)
        header.version = 4
        return

# -----------------------------------------------------------------------------
# --- DtkFileV5
# ---
# --- "Emod info added": emod_info added to header
# -----------------------------------------------------------------------------

class DtkFileV5(DtkFileV4):
    def __init__(self, header=None, filename='', handle=None):
        if header is None:
            header = DtkHeader()
            version5_params = {
                'emod_info': {
                    'emod_major_version': 0,
                    'emod_minor_version': 0,
                    'emod_revision_number': 0,
                    'ser_pop_major_version': 0,
                    'ser_pop_minor_version': 0,
                    'ser_pop_patch_version': 0,
                    'emod_build_date': "Mon Jan 1 00:00:00 1970",
                    'emod_builder_name': "",
                    'emod_sccs_branch': 0,
                    'emod_sccs_date': "Mon Jan 1 00:00:00 1970"
                }
            }
            header.update(version5_params)
        super(DtkFileV5, self).__init__(header, filename, handle)
        header.version = 5
        return

# -----------------------------------------------------------------------------
# --- DtkHeaderV6
# -----------------------------------------------------------------------------

class DtkHeaderV6(support.SerialObject):
    """
    The header for V6 is quite different because we distinguish the different types
    of chunks - sim, node, human collection.  It also specifies the compression type
    for each type of chunk separately.

    The 'human_num_humans' was added to the format to support this python code.
    It helps the code to know how many humans are in a particular chunk so that
    we can hide that the humans for one node are actually in different collections.
    """
    def __init__(self, dictionary=None):
        if dictionary is None:
            dictionary = {
                "version": 6,
                "author": "IDM",
                "tool": "DTK",
                "date": time.strftime('%a %b %d %H:%M:%S %Y'),
                "emod_info": {
                    'emod_major_version': 0,
                    'emod_minor_version': 0,
                    'emod_revision_number': 0,
                    'ser_pop_major_version': 0,
                    'ser_pop_minor_version': 0,
                    'ser_pop_patch_version': 0,
                    'emod_build_date': "Mon Jan 1 00:00:00 1970",
                    'emod_builder_name': "",
                    'emod_sccs_branch': 0,
                    'emod_sccs_date': "Mon Jan 1 00:00:00 1970"
                },
                "sim_compression": "NON",
                "sim_chunk_size": "0000000000000000",
                "node_suids": [],
                "node_compressions": [],
                "node_chunk_sizes": [],
                "human_compressions": [],
                "human_node_suids": [],
                "human_num_humans": [],
                "human_chunk_sizes": []
            }
        super(DtkHeaderV6, self).__init__(dictionary)
        return

    def __str__(self):
        text = json.dumps(self, separators=(',', ':'))
        return text

    def __len__(self):
        length = len(self.__str__())
        return length

# -----------------------------------------------------------------------------
# --- DtkFileV6
# ---
# --- "Per-node/human compression and chunk sizes": sim_compression, sim_chunk_size, node_compressions,
# --- node_chunk_sizes, human_compressions, human_node_suids, human_chunk_sizes added to header
# -----------------------------------------------------------------------------

class DtkFileV6(object):
    """
    The V6 file moves the humans out of the JSON serialized for the node and puts
    them into their own chunks.  This helps to reduce the size of the JSON for
    one node and allows the memory for one collection of humans be freed before
    we get the next set.  This greatly reduces the peak memory usage when processing
    populations that require lots of memory.
    """
    class Chunk(object):
        """
        Chunk represnts a compressed chunk of data in a V6 serialized population file.
        In the code, _json and _chunk are mutually exclusive - only one is populated at a time.

        Args:
            filename (str): The name of the file being read (for error messages).
            obj_type_str (str): The type of object in the chunk (for error messages).
            v6_compression_str (str): The V6 compression string for the chunk.
            node_suid (int): The SUID of the node the chunk belongs to.
            chunk_size (int): The size of the chunk in bytes.
            chunk (bytes): The compressed chunk data.
        """
        def __init__(self,
                     filename,
                     obj_type_str,
                     v6_compression_str,
                     node_suid,
                     chunk_size,
                     chunk):
            if chunk is None and chunk_size != 0:
                msg = f"Chunk is None but chunk size is {chunk_size} for {obj_type_str} chunk of file '{filename}'"
                raise UserWarning(msg)
            elif (chunk is not None) and (len(chunk) != chunk_size):
                msg = f"Only read {len(chunk)} bytes of {chunk_size} for {obj_type_str} chunk of file '{filename}'"
                raise UserWarning(msg)
            
            self._v6_compression_str = v6_compression_str
            self._node_suid = node_suid
            self._chunk_size = chunk_size
            self._chunk = chunk
            self._json = None
            return

        def get_json(self):
            """
            Return the JSON dictionary for the chunk, uncompressing and parsing it if necessary.
            """
            if self._json is None:
                old_compression_type = _compression_type_v6_to_old(self._v6_compression_str)
                uncomp_data = str(uncompress(self._chunk, old_compression_type), 'utf-8')
                try:
                    json_data = json.loads(uncomp_data, object_hook=support.SerialObject)
                except Exception:
                    raise UserWarning("Could not parse JSON in chunk with size {0}".format(self._chunk_size))
                self._json = json_data
                self._chunk = None
                self._chunk_size = 0
                gc.collect()
            return self._json

        def set_json(self, json_data):
            """
            Replace the existing JSON with the provided JSON dictionary.
            Also compresses and stores the chunk.
            """
            self._json = json_data
            self.store()
            return
        
        def store(self):
            """
            Compress and store the JSON dictionary as a chunk.
            """
            if self._chunk is None:
                json_data = json.dumps(self._json, separators=(',', ':'))
                self._v6_compression_str = _determine_v6_compression_type(json_data)
                old_compression_type = _compression_type_v6_to_old(self._v6_compression_str)
                self._chunk = compress(json_data.encode(), old_compression_type)
                self._chunk_size = len(self._chunk)
                self._json = None
                gc.collect()
            return

        @property
        def v6_compression_str(self):
            """
            Return the V6 compression string for the chunk - NON, LZ4, SNA.
            """
            return self._v6_compression_str

        @property
        def node_suid(self):
            """
            Return the SUID of the node the chunk belongs to - not the external ID.
            """
            return self._node_suid

        @property
        def chunk_size(self):
            """
            Return the size of the chunk in bytes.
            """
            return self._chunk_size

        @property
        def chunk(self):
            """
            Return the compressed chunk data.
            """
            return self._chunk

    class HumanCollectionChunkV6(Chunk):
        """
        This represents one collection of humans in a node.

        Args:
            filename (str): The name of the file being read (for error messages).
            obj_type_str (str): The type of object in the chunk (for error messages).
            v6_compression_str (str): The V6 compression string for the chunk.
            node_suid (int): The SUID of the node the chunk belongs to.
            num_humans (int): The number of humans in the collection.
            chunk_size (int): The size of the chunk in bytes.
            chunk (bytes): The compressed chunk data.
        """
        def __init__(self,
                     filename,
                     obj_type_str,
                     v6_compression_str,
                     node_suid,
                     num_humans,
                     chunk_size,
                     chunk):
            super(DtkFileV6.HumanCollectionChunkV6, self).__init__(filename,
                                                         obj_type_str,
                                                         v6_compression_str,
                                                         node_suid,
                                                         chunk_size,
                                                         chunk)
            self._num_humans = num_humans
            return

        def get_json(self):
            """
            Return an list of JSON IndividualHuman dictionaries.
            """
            json_data = super(DtkFileV6.HumanCollectionChunkV6, self).get_json()
            return json_data['human_collection']

        def set_json(self, human_list):
            """
            Replace the existing JSON with the provided list of IndividualHuman dictionaries.
            """
            self._json = {}
            self._json['human_collection'] = human_list
            self._num_humans = len(human_list)
            self.store()
            return

        @property
        def num_humans(self):
            """
            Return the number of humans in the collection.
            """
            return self._num_humans

    class NodeV6(MutableMapping):
        """
        NodeV6 represents one node in a V6 serialized population file.
        The purpose of this class is to delay loading the full JSON for the node
        until it is actually needed and to provide a backwards compatible interface
        to the human data via the individualHumans property.

        Implementation notes:
            - This class inherits from MutableMapping to provide dictionary-like
            access to the node JSON data.  The individualHumans property is
            handled specially to return the human list.
            - __get_item__, __setitem__, __delitem__ each handle the key 'individualHumans'
              specially to return or raise an error as appropriate.  This helps to provide
              a backwards compatible interface to the human data.

        Args:
            parent (DtkFileV6): The parent DtkFileV6 object.
            node_chunk (DtkFileV6.Chunk): The chunk containing the node data.
            human_chunk_list (list of DtkFileV6.HumanCollectionChunkV6):
                The list of chunks containing the human data for the node.
        """
        def __init__(self, parent, node_chunk, human_chunk_list):
            super(DtkFileV6.NodeV6, self).__init__()
            self.__parent__ = parent
            self._node_chunk = node_chunk
            self._human_list = DtkFileV6.HumanListV6(self, human_chunk_list)
            self._json = None
            return

        def __getitem__(self, key):
            """
            Return the value for the given key in the node JSON dictionary.
            If the key is 'individualHumans', return the human list instead.
            """
            if key == 'individualHumans':
                return self._human_list
            else:
                self.load()
                return self._json[key]
        
        def __setitem__(self, key, value):
            """
            Set the value for the given key in the node JSON dictionary.
            Cannot set the 'individualHumans' key directly.
            """
            self.load()
            if key == 'individualHumans':
                self.individualHumans = value
            else:
                self._json[key] = value
        
        def __delitem__(self, key):
            """
            Delete the given key from the node JSON dictionary.
            Cannot delete the 'individualHumans' key.
            """
            if key == 'individualHumans':
                raise RuntimeError("Cannot set individualHumans property directly")
            self.load()
            del self._json[key]
        
        def __iter__(self):
            self.load()
            return iter(self._json)
        
        def __len__(self):
            self.load()
            return len(self._json)

        def __repr__(self):
            self.load()
            return repr(self._json)

        def keys(self):
            """
            Return the keys of the node JSON dictionary as a list.
            """
            self.load()
            return list(super(DtkFileV6.NodeV6, self).keys())

        def get_json(self):
            self.load()
                
            tmp_json = copy.deepcopy(self._json)

            # remove member variables from json
            keys_to_remove = ['__parent__', '_node_chunk', '_human_list', '_json']
            for key in keys_to_remove:
                del tmp_json[key]
                
            return tmp_json
            
            
        def load(self):
            """
            Load the node JSON dictionary from the chunk if it is not already loaded.
            """
            if self._json is None:
                keys = list(self.__dict__.keys())
                values = list(self.__dict__.values())
                tmp_json = self._node_chunk.get_json()
                self.__dict__ = tmp_json
                for key, value in zip(keys, values):
                    self.__dict__[key] = value
                self._json = tmp_json
                self._node_chunk._chunk = None
                self._node_chunk._chunk_size = 0
                gc.collect()
            return

        def store(self):
            """
            Store the node JSON dictionary back to the chunk if it is loaded.

            Implementation note:
                We need to temporarily remove references to the member variables of
                this class from the _json/__dict__ before storing it back to the chunk.
                This keeps us from compressing the wrong stuff.  We add them back afterwards.
            """
            if self._json is not None:
                # save member variables
                parent = self.__parent__
                node_chunk = self._node_chunk
                human_list = self._human_list
                tmp_json = self._json

                # remove member variables from json
                keys_to_remove = ['__parent__', '_node_chunk', '_human_list', '_json']
                for key in keys_to_remove:
                    del tmp_json[key]

                # compress json
                node_chunk.set_json(tmp_json)

                # restore member variables
                self.__parent__ = parent
                self._node_chunk = node_chunk
                self._human_list = human_list

                 # clear json to free memory
                self._json = None
                gc.collect()
            return
        
        def _clear_human_list(self):
            """
            Clear the human list for the node.
            """
            self.__parent__._remove_humans_for_node(self._node_chunk.node_suid)
            self._human_list = DtkFileV6.HumanListV6(node=self, human_chunk_list=[])
            return

        @property
        def individualHumans(self):
            """
            Return a list of IndividualHuman dictionaries for the node.
            """
            return self._human_list

        @individualHumans.setter
        def individualHumans(self, json_dict_list):
            self._clear_human_list()
            human_chunk = DtkFileV6.HumanCollectionChunkV6(
                filename="no file",
                obj_type_str="human",
                v6_compression_str=None,
                node_suid=self._node_chunk.node_suid,
                num_humans=0,
                chunk_size=0,
                chunk=None)
            human_chunk.set_json(json_dict_list)
            self.__parent__._human_chunks.append(human_chunk)
            self._human_list._add_human_chunk(human_chunk)
            print(f"Set {len(json_dict_list)} humans for node SUID {self._node_chunk.node_suid} and num_humans-{len(self._human_list)}")
            return

    class NodeListV6(object):
        """
        The NodeListV6 provides an interface to a list of NodeV6 objects.
        The main purpose of this class is to manage loading and unloading
        the node data when iterating over the nodes.
        """
        def __init__(self, parent):
            self.__parent__ = parent
            self._node_list = []
            return

        def __iter__(self):
            index = 0
            while index < len(self):
                self._node_list[index].load()
                yield self.__getitem__(index)
                self._node_list[index].store()
                index += 1

        def __getitem__(self, index):
            node = self._node_list[index]
            node.load()
            return node

        def __setitem__(self, index, node):
            self._node_list[index] = node
            return

        def __len__(self):
            length = len(self._node_list)
            return length
        
        def append(self, node_chunk):
            self._node_list.append(node_chunk)
            return

    class HumanListV6(object):
        """
        A HumanListV6 provides an interface to a list of IndividualHuman dictionaries
        that may be stored in multiple HumanCollectionChunkV6 chunks.  The purpose of
        this class is to manage loading and unloading the human collection chunks
        when iterating over the humans.  It hides the fact that the humans for one node
        may be stored in multiple collections.
        """
        def __init__(self, node, human_chunk_list):
            self._node = node
            self._human_chunk_list = human_chunk_list
            self._num_humans = 0
            for human_chunk in self._human_chunk_list:
                self._num_humans += human_chunk.num_humans
            self._human_chunk_index = 0
            self._current_collection = None
            self._current_min_index = 0
            self._current_max_index = 0
            self.__init_current()
            return

        def __init_current(self):
            """
            Initialize the current human collection chunk.
            """
            if (len(self._human_chunk_list) > 0) and (self._current_collection is None):
                self._current_collection = self._human_chunk_list[0].get_json()
                self._current_min_index = 0
                self._current_max_index = len(self._current_collection) - 1
                if len(self._current_collection) != self._human_chunk_list[0].num_humans:
                    msg = f"Number of humans in first human chunk [{len(self._current_collection)}]"
                    msg += f" does not match num_humans attribute [{self._human_chunk_list[0].num_humans}]"
                    raise RuntimeError(msg)
            return

        def _add_human_chunk(self, human_chunk):
            """
            Add a new human collection chunk to the list.
            """
            self._human_chunk_list.append(human_chunk)
            self._num_humans += human_chunk.num_humans
            self.__init_current()
            return

        def __iter__(self):
            human_index = 0
            self.__update_current_collection__(human_index)
            while human_index < len(self):
                yield self.__getitem__(human_index)
                human_index += 1

        def __update_current_collection__(self, human_index):
            """
            Update/load the current human collection chunk to include the specified human index.
            0-based human_index is the index of the human in the full list of humans for the node.
            0-based _current_min_index and _current_max_index are the min and max indices of the
            currently loaded human collection chunk and are inclusive.
            """
            print("Updating current collection...")
            print(human_index, self._current_min_index, self._current_max_index,self._num_humans)
            if self._num_humans == 0:
                return
            
            if human_index < self._current_min_index:
                while human_index < self._current_min_index:
                    self._human_chunk_list[ self._human_chunk_index].store()
                    self._human_chunk_index -= 1
                    if self._human_chunk_index < 0:
                        raise IndexError("Index {0} is out of range for human collection".format(human_index))
                    self._current_collection = self._human_chunk_list[self._human_chunk_index].get_json()
                    self._current_max_index = self._current_min_index - 1
                    self._current_min_index = self._current_max_index - len(self._current_collection) + 1
                    if len(self._current_collection) != self._human_chunk_list[self._human_chunk_index].num_humans:
                        raise RuntimeError("Number of humans in first human chunk does not match num_humans attribute")
            else:
                while human_index > self._current_max_index:
                    self._human_chunk_list[ self._human_chunk_index ].store()
                    self._human_chunk_index += 1
                    if self._human_chunk_index >= len(self._human_chunk_list):
                        raise IndexError("Index {0} is out of range for human collection".format(human_index))
                    self._current_collection = self._human_chunk_list[self._human_chunk_index].get_json()
                    self._current_min_index = self._current_max_index + 1
                    self._current_max_index = self._current_min_index + len(self._current_collection) - 1
                    if len(self._current_collection) != self._human_chunk_list[self._human_chunk_index].num_humans:
                        raise RuntimeError(f"current collection = {len(self._current_collection)} but num_humans = {self._human_chunk_list[self._human_chunk_index].num_humans}")
            return
        
        def __getitem__(self, human_index):
            """
            Return the IndividualHuman dictionary at the specified index.
            """
            if human_index < self._current_min_index or human_index > self._current_max_index:
                self.__update_current_collection__(human_index)
            return self._current_collection[human_index - self._current_min_index]

        def __setitem__(self, human_index, value):
            """
            Set the IndividualHuman dictionary at the specified index.
            """
            if human_index < self._current_min_index or human_index > self._current_max_index:
                self.__update_current_collection__(human_index)
            self._current_collection[human_index - self._current_min_index] = value
            return
        
        def __len__(self):
            return self._num_humans

        def append(self, human_dict):
            human_index = self._num_humans
            if self._human_chunk_index != (len(self._human_chunk_list) - 1):
                self._human_chunk_list[ self._human_chunk_index ].store()
                self._human_chunk_index = len(self._human_chunk_list) - 1
                self._current_collection = self._human_chunk_list[self._human_chunk_index].get_json()
                self._current_min_index = self._num_humans - len(self._current_collection)
                self._current_max_index = self._num_humans - 1
            self._current_collection.append(human_dict)
            self._current_max_index += 1
            self._num_humans += 1
            self._human_chunk_list[ self._human_chunk_index ]._num_humans += 1
            print(f"Appended human at index {human_index}, total num_humans-{self._num_humans} min_index={self._current_min_index} max_index={self._current_max_index} num_colleciton={len(self._current_collection)}")

    def __init__(self, header=None, filename='', handle=None):
        """
        Initialize a DtkFileV6 object from the provided header and file handle.
        This should read the file and create chunk objects for the simulation, nodes,
        and humans.  It will not uncompress or parse any of the JSON data until it is needed.

        Args:
            header (DtkHeaderV6): The header for the file.
            filename (str): The name of the file being read (for error messages).
            handle (file-like object): The file handle to read the data from.
        """
        if header is None:
            header = DtkHeaderV6()
        self.__header__ = header
        self._sim_chunk = None
        self._node_chunks = []
        self._human_chunks = []
        self._nodes = DtkFileV6.NodeListV6(self)

        if handle is not None:
            sim_chunk_size = int(header.sim_chunk_size, 16)
            sim_chunk_data = handle.read(sim_chunk_size)
            self._sim_chunk = DtkFileV6.Chunk(filename,
                                              "sim",
                                              header.sim_compression,
                                              -1,
                                              sim_chunk_size,
                                              sim_chunk_data)

            for index, size_string in enumerate(header.node_chunk_sizes):
                v6_compression_str = header.node_compressions[index]
                node_suid = int(header.node_suids[index], 16)
                chunk_size = int(size_string, 16)
                chunk_data = handle.read(chunk_size)
                node_chunk = DtkFileV6.Chunk(filename,
                                             "node",
                                             v6_compression_str,
                                             node_suid,
                                             chunk_size,
                                             chunk_data)
                self._node_chunks.append(node_chunk)

            for index, size_string in enumerate(header.human_chunk_sizes):
                v6_compression_str = header.human_compressions[index]
                node_suid_str = header.human_node_suids[index]
                num_humans_str = header.human_num_humans[index]
                node_suid = int(node_suid_str, 16)
                num_humans = int(num_humans_str, 16)
                chunk_size = int(size_string, 16)
                chunk_data = handle.read(chunk_size)
                human_chunk = DtkFileV6.HumanCollectionChunkV6(filename,
                                                     "human",
                                                     v6_compression_str,
                                                     node_suid,
                                                     num_humans,
                                                     chunk_size,
                                                     chunk_data)
                self._human_chunks.append(human_chunk)

            for node_chunk in self._node_chunks:
                human_chunk_list = []
                for human_chunk in self._human_chunks:
                    if human_chunk.node_suid == node_chunk.node_suid:
                        human_chunk_list.append(human_chunk)
                self._nodes.append(DtkFileV6.NodeV6(self, node_chunk, human_chunk_list))

        return

    def _remove_humans_for_node(self, node_suid):
        """
        Remove all human chunks for the specified node SUID.
        """
        new_human_chunks = []
        for human_chunk in self._human_chunks:
            if human_chunk.node_suid != node_suid:
                new_human_chunks.append(human_chunk)
        self._human_chunks = new_human_chunks
        return

    @property
    def header(self):
        return self.__header__

    # Optional header entries
    @property
    def author(self):
        return self.__header__.author if 'author' in self.__header__ else ''

    @author.setter
    def author(self, value):
        self.__header__['author'] = str(value)
        return

    @property
    def date(self):
        return self.__header__.date if 'date' in self.__header__ else ''

    @date.setter
    def date(self, value):
        self.__header__['date'] = str(value)

    @property
    def tool(self):
        return self.__header__.tool if 'tool' in self.__header__ else ''

    @tool.setter
    def tool(self, value):
        self.__header__['tool'] = str(value)
        return

    @property
    def version(self):
        return self.__header__.version

    @property
    def nodes(self):
        """
        Return the list of NodeV6 objects in the file.
        Do not try to access the nodes via the simulation property of this class.
        This keeps it backwards compatible.
        """
        return self._nodes

    def _sync_header(self):
        self._sim_chunk.store()
        for node in self.nodes:
            node.store()
        for human_chunk in self._human_chunks:
            human_chunk.store()
        
        self.__header__['date'] = time.strftime('%a %b %d %H:%M:%S %Y')
        self.__header__['sim_compression'] = self._sim_chunk.v6_compression_str
        self.__header__['sim_chunk_size'] = format(self._sim_chunk.chunk_size, '016x')
        self.__header__['node_compressions'] = []
        self.__header__['node_chunk_sizes'] = []
        self.__header__['node_suids'] = []
        for node_chunk in self._node_chunks:
            self.__header__['node_compressions'].append(node_chunk.v6_compression_str)
            self.__header__['node_chunk_sizes'].append(format(node_chunk.chunk_size, '016x'))
            self.__header__['node_suids'].append(format(node_chunk.node_suid, '016x'))
        self.__header__['human_compressions'] = []
        self.__header__['human_chunk_sizes'] = []
        self.__header__['human_node_suids'] = []
        self.__header__['human_num_humans'] = []
        for human_chunk in self._human_chunks:
            self.__header__['human_compressions'].append(human_chunk.v6_compression_str)
            self.__header__['human_chunk_sizes'].append(format(human_chunk.chunk_size, '016x'))
            self.__header__['human_node_suids'].append(format(human_chunk.node_suid, '016x'))
            self.__header__['human_num_humans'].append(format(human_chunk.num_humans, '016x'))
        return

    @property
    def simulation(self):
        """
        Return the simulation JSON dictionary.  Do not try to access the nodes
        from this dictionary - use the nodes property of this class instead.
        """
        return self._sim_chunk.get_json()

    @simulation.setter
    def simulation(self, value):
        value["nodes"] = []
        self._sim_chunk.set_json(value)
        return

# -----------------------------------------------------------------------------
# --- Reading Functions
# -----------------------------------------------------------------------------

def read(filename):

    new_file = None
    with open(filename, 'rb') as handle:
        __check_magic_number__(handle)
        header = __read_header__(handle)
        if header.version == 1:
            new_file = DtkFileV1(header, filename=filename, handle=handle)
        elif header.version == 2:
            new_file = DtkFileV2(header, filename=filename, handle=handle)
        elif header.version == 3:
            new_file = DtkFileV3(header, filename=filename, handle=handle)
        elif header.version == 4:
            new_file = DtkFileV4(header, filename=filename, handle=handle)
        elif header.version == 5:
            new_file = DtkFileV5(header, filename=filename, handle=handle)
        elif header.version == 6:
            new_file = DtkFileV6(header, filename=filename, handle=handle)
        else:
            raise UserWarning('Unknown serialized population file version: {0}'.format(header.version))

    return new_file


def __check_magic_number__(handle):
    magic = handle.read(4).decode()
    if magic != IDTK:
        raise UserWarning("File has incorrect magic 'number': '{0}'".format(magic))
    return


def __read_header__(handle):

    size_string = handle.read(12)
    header_size = int(size_string)
    __check_header_size__(header_size)
    header_text = handle.read(header_size)
    header_json = __try_parse_header_text__(header_text)

    if 'metadata' in header_json:
        header_json = header_json["metadata"]

    if 'version' not in header_json:
        header_json['version'] = 1

    header = None
    if header_json['version'] < 6:
        header = DtkHeader(header_json)
        if header.version < 2:
            header.engine = SNAPPY if header.compressed else NONE
            header.chunkcount = 1
            header.chunksizes = [header.bytecount]

        __check_version__(header.version)

        if header.version < 4:
            header.engine = header.engine.upper()
            __check_chunk_sizes__(header.chunksizes)
        else:
            header['engine'] = header.compression.upper()
            __check_chunk_sizes__(header.chunksizes)
    else:
        header = DtkHeaderV6(header_json)
        __check_version__(header.version)
        __check_chunk_sizes_v6__(header)

    return header


def __check_header_size__(header_size):
    if header_size <= 0:
        raise UserWarning("Invalid header size: {0}".format(header_size))
    return


def __try_parse_header_text__(header_text):
    try:
        header_json = json.loads(header_text)
    except ValueError as err:
        raise UserWarning("Couldn't decode JSON header '{0}'".format(err))
    return header_json


def __check_version__(version):
    if version <= 0 or version > MAX_VERSION:
        raise UserWarning("Unknown version: {0}".format(version))
    return


def __check_chunk_sizes__(chunk_sizes):
    for size in chunk_sizes:
        if size <= 0:
            raise UserWarning("Invalid chunk size: {0}".format(size))
    return

def __check_chunk_sizes_v6__(header):
    #        "version": 6,
    #        "author": "IDM",
    #        "tool": "DTK",
    #        "date": "Day Mon day HH:MM:SS year",
    #        "emod_info": {},
    #        "sim_compression": "LZ4",
    #        "sim_chunk_size": "FFFFFFFF",
    #        "node_suids": [ "00000001", "00000002", "00000002", ..., "00000002" ],
    #        "node_compressions": [ "NON", "LZ4", "SNA", ..., "SNA" ]
    #        "node_chunk_sizes": [ "FFFFFFFF", "FFFFFFFF", "FFFFFFFF", ..., "FFFFFFFF" ],
    #        "human_compressions": [ "NON", "LZ4", "SNA", ..., "SNA" ]
    #        "human_node_suids": [ "00000001", "00000002", "00000002", ..., "00000002" ],
    #        "human_num_humans": [ "0000000A", "00000014", "00000014", ..., "00000014" ],
    #        "human_chunk_sizes": [ "FFFFFFFF", "FFFFFFFF", "FFFFFFFF", ..., "FFFFFFFF" ]

    sim_chunk_size = int(header["sim_chunk_size"], 16)
    if sim_chunk_size <= 0:
        raise UserWarning("Invalid 'sim_chunk_size': {0}".format(sim_chunk_size))

    for size_string in header["node_chunk_sizes"]:
        size = int(size_string, 16)
        if size <= 0:
            raise UserWarning("Invalid 'node_chunk_size': {0}".format(size))

    for size_string in header["human_chunk_sizes"]:
        size = int(size_string, 16)
        if size <= 0:
            raise UserWarning("Invalid 'human_chunk_size': {0}".format(size))

    return

# -----------------------------------------------------------------------------
# --- Writing Functions
# -----------------------------------------------------------------------------

def write(dtk_file, filename):

    dtk_file._sync_header()

    with open(filename, 'wb') as handle:
        __write_magic_number__(handle)
        print("Writing file: {0}".format(filename))
        if dtk_file.version <= 3:
            header = json.dumps({'metadata': dtk_file.header}, separators=(',', ':'))
        else:
            header = json.dumps(dtk_file.header, separators=(',', ':')).replace('"engine"', '"compression"')

        __write_header_size__(len(header), handle)
        __write_header__(header, handle)
        if dtk_file.version <= 5:
            __write_chunks__(dtk_file.chunks, handle)
        else:
            handle.write(dtk_file._sim_chunk._chunk)
            for node_chunk in dtk_file._node_chunks:
                handle.write(node_chunk._chunk)
            for human_chunk in dtk_file._human_chunks:
                handle.write(human_chunk._chunk)

    return


def __write_magic_number__(handle):
    handle.write('IDTK'.encode())
    return


def __write_header_size__(size, handle):
    size_string = '{:>12}'.format(size)     # decimal value right aligned in 12 character space
    handle.write(size_string.encode())
    return


def __write_header__(string, handle):
    handle.write(string.encode())
    return


def __write_chunks__(chunks, handle):
    for chunk in chunks:
        handle.write(chunk if type(chunk) is bytes else chunk.encode())
    return
