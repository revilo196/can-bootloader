from msgpack import Packer

class CommandType:
    JumpToMain = 1
    CRCReginon = 2
    Write = 3
    Read = 4
    UpdateConfig = 6
    SaveConfig = 7

def encode_command(command_code, *arguments):
    """
    Encodes a command of the given type with given arguments.
    """
    p = Packer(use_bin_type=True)
    obj = list(arguments)
    return p.pack(command_code) +  p.pack(obj)

def encode_crc_region(address, length):
    """
    Encodes the command to request the CRC of a region in flash.
    """
    return encode_command(CommandType.CRCReginon, address, length)

def encode_write_flash(data, adress, device_class):
    """
    Encodes the command to write the given data at the given adress in a
    messagepack byte object.
    """
    return encode_command(CommandType.Write, adress, device_class, data)

def encode_read_flash(aderess, length):
    """
    Encodes the command to read the flash at given address.
    """
    return encode_command(CommandType.Read, adress, length)

def encode_update_config(data):
    """
    Encodes the command to update the config from given MessagePack data.
    """
    return encode_command(CommandType.UpdateConfig, data)

def encode_save_config():
    """
    Encodes the command to save the config to flash.
    """
    return encode_command(CommandType.SaveConfig)

def encode_jump_to_main():
    """
    Encodes the command to jump to application using MessagePack.
    """
    return encode_command(CommandType.JumpToMain)
