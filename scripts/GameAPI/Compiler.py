import os
import struct
from typing import List, Tuple, Dict
from PIL import Image
import heatshrink2
import esprima

# Constants
MAGIC_NUMBER = b"ESPG"
VERSION = 0x02
HEADER_SIZE = 20
ASSET_ENTRY_SIZE = 16
EVENT_ENTRY_SIZE = 12

# Asset Types
ASSET_SPRITE = 0
ASSET_IMAGE = 1

# Image Formats (Flags)
FORMAT_RGB565 = 0
FORMAT_INDEXED_8 = 1
FORMAT_INDEXED_4 = 2
FORMAT_PNG = 3

# Event Types
EVENT_TOUCH_PRESS = 0x01

# Bytecode Opcodes
OP_NOP = 0x00
OP_MOVE_SPRITE = 0x01
OP_SET_ANIMATION = 0x02
OP_IF_BUTTON = 0x03  # Not used currently
OP_SET_VAR = 0x04    # Not used currently
OP_ADD_VAR = 0x05    # Not used currently
OP_ON_EVENT = 0x06
OP_RETURN = 0x07
OP_JUMP = 0x08       # Jump to offset
OP_DRAW_PIXEL = 0x09
OP_DRAW_LINE = 0x0A
OP_DRAW_RECT = 0x0B
OP_LOAD_INT = 0x10   # Push integer onto stack
OP_LOAD_STRING = 0x11  # Push string onto stack
OP_LOAD_VAR = 0x12   # Push variable value onto stack
OP_STORE_VAR = 0x13  # Pop value and store in variable
OP_ADD = 0x14        # Pop two values, add, push result
OP_SUB = 0x15        # Pop two values, subtract, push result
OP_IF_EQ = 0x16      # If top two stack values equal, continue, else jump
OP_END = 0xFF

class EspgCompilerError(Exception):
    pass

class EspgCompiler:
    def __init__(self, config_js_file: str):
        if not os.path.exists(config_js_file):
            raise EspgCompilerError(f"Config file '{config_js_file}' not found")
        self.config_js_file = os.path.abspath(config_js_file)
        self.variable_table = {}  # Maps variable names to indices
        self.next_var_index = 0   # Next available index
        self.initial_values = {}  # Maps var_index to initial integer values
        self.config = self.parse_config()
        self.asset_folder = os.path.abspath(os.path.join(os.path.dirname(self.config_js_file),
                                                        self.config.get("asset_folder", "assets")))
        self.output_file = os.path.abspath(os.path.join(os.path.dirname(self.config_js_file),
                                                       self.config.get("output", "game.espg")))
        self.compress_logic = self.config.get("compress_logic", True)

    def parse_config(self) -> Dict:
        with open(self.config_js_file, "r") as f:
            js_code = f.read()

        try:
            ast = esprima.parseScript(js_code)
        except Exception as e:
            raise EspgCompilerError(f"Failed to parse JS config: {e}")

        config = {}
        game_logic = []

        def extract_object(node):
            obj = {}
            for prop in node.properties:
                key = prop.key.name
                if prop.value.type == "Literal":
                    obj[key] = prop.value.value
                elif prop.value.type == "ArrayExpression":
                    obj[key] = [extract_element(elem) for elem in prop.value.elements]
                elif prop.value.type == "ObjectExpression":
                    obj[key] = extract_object(prop.value)
            return obj

        def extract_element(node):
            if node.type == "Literal":
                return node.value
            elif node.type == "ObjectExpression":
                return extract_object(node)
            return None

        for node in ast.body:
            if node.type == "VariableDeclaration":
                for decl in node.declarations:
                    var_name = decl.id.name
                    if var_name not in self.variable_table:
                        self.variable_table[var_name] = self.next_var_index
                        if decl.init and decl.init.type == "Literal":
                            if isinstance(decl.init.value, int):
                                self.initial_values[self.next_var_index] = decl.init.value
                            elif isinstance(decl.init.value, str):
                                self.initial_values[self.next_var_index] = decl.init.value  # Store string for interpreter
                        self.next_var_index += 1
                    if decl.id.name == "config" and decl.init.type == "ObjectExpression":
                        config = extract_object(decl.init)
                    elif decl.id.name == "assets" and decl.init.type == "ArrayExpression":
                        config["assets"] = [extract_object(elem) for elem in decl.init.elements]
            elif node.type == "ExpressionStatement" or node.type == "FunctionDeclaration":
                game_logic.append(node)

        if not config:
            raise EspgCompilerError("No 'config' object found in JS file")
        config["game_logic_ast"] = game_logic
        return config

    def convert_image(self, filepath: str, format: int) -> Tuple[bytes, int]:
        try:
            img = Image.open(filepath).convert("RGB")
        except Exception as e:
            raise EspgCompilerError(f"Failed to load image '{filepath}': {e}")

        if format == FORMAT_RGB565:
            pixels = [(r & 0xF8) << 8 | (g & 0xFC) << 3 | (b >> 3) for r, g, b in img.getdata()]
            return struct.pack(f"<{len(pixels)}H", *pixels), FORMAT_RGB565
        elif format == FORMAT_INDEXED_8:
            img = img.convert("P", palette=Image.ADAPTIVE, colors=256)
            palette = img.getpalette()[:256*3]
            return bytes(img.tobytes()) + bytes(palette), FORMAT_INDEXED_8
        elif format == FORMAT_INDEXED_4:
            img = img.convert("P", palette=Image.ADAPTIVE, colors=16)
            palette = img.getpalette()[:16*3]
            return bytes(img.tobytes()) + bytes(palette), FORMAT_INDEXED_4
        elif format == FORMAT_PNG:
            with open(filepath, "rb") as f:
                return f.read(), FORMAT_PNG
        else:
            raise EspgCompilerError(f"Unsupported format: {format}")

    def process_assets(self) -> Tuple[List[bytes], bytes]:
        if not os.path.exists(self.asset_folder):
            raise EspgCompilerError(f"Asset folder '{self.asset_folder}' not found")

        assets = []
        compressed_assets = bytearray()
        offset = HEADER_SIZE + (len(self.config.get("assets", [])) * ASSET_ENTRY_SIZE)

        for asset in self.config.get("assets", []):
            filename = asset["name"]
            filepath = os.path.join(self.asset_folder, filename)
            if not os.path.exists(filepath):
                raise EspgCompilerError(f"Asset '{filepath}' not found")

            asset_type = ASSET_SPRITE if asset.get("type", "sprite") == "sprite" else ASSET_IMAGE
            format_name = asset.get("format", "rgb565").lower()
            format_map = {"rgb565": FORMAT_RGB565, "indexed8": FORMAT_INDEXED_8, "indexed4": FORMAT_INDEXED_4, "png": FORMAT_PNG}
            format = format_map.get(format_name, FORMAT_RGB565)

            raw_data, flag = self.convert_image(filepath, format)
            compressed_data = heatshrink2.encode(raw_data, window_sz2=8, lookahead_sz2=4)

            entry = struct.pack("<BIII3B", asset_type, offset, len(compressed_data), len(raw_data), flag, 0, 0)
            assets.append(entry)
            compressed_assets.extend(compressed_data)
            offset += len(compressed_data)

        return assets, compressed_assets

    def encode_string(self, s: str) -> bytes:
        encoded = s.encode("utf-8") + b"\x00"
        return encoded

    def compile_expression(self, node, bytecode):
        if node.type == "Literal":
            if isinstance(node.value, int):
                bytecode.extend(struct.pack("<B", OP_LOAD_INT))
                bytecode.extend(struct.pack("<i", node.value))
            elif isinstance(node.value, str):
                bytecode.extend(struct.pack("<B", OP_LOAD_STRING))
                bytecode.extend(self.encode_string(node.value))
            else:
                raise EspgCompilerError(f"Unsupported literal type: {type(node.value)}")
        elif node.type == "Identifier":
            var_name = node.name
            if var_name not in self.variable_table:
                raise EspgCompilerError(f"Undefined variable: {var_name}")
            var_index = self.variable_table[var_name]
            bytecode.extend(struct.pack("<B", OP_LOAD_VAR))
            bytecode.extend(struct.pack("<B", var_index))
        elif node.type == "BinaryExpression":
            self.compile_expression(node.left, bytecode)
            self.compile_expression(node.right, bytecode)
            if node.operator == "+":
                bytecode.extend(struct.pack("<B", OP_ADD))
            elif node.operator == "-":
                bytecode.extend(struct.pack("<B", OP_SUB))
            elif node.operator == "===":
                bytecode.extend(struct.pack("<B", OP_IF_EQ))  # Placeholder for equality check
                # Offset will be patched later in conditional handling
            else:
                raise EspgCompilerError(f"Unsupported operator: {node.operator}")
        elif node.type == "ConditionalExpression":
            # Compile test condition
            if node.test.type == "BinaryExpression" and node.test.operator == "===":
                self.compile_expression(node.test.left, bytecode)
                self.compile_expression(node.test.right, bytecode)
            else:
                raise EspgCompilerError("Only === supported in conditional test")

            # Placeholder for OP_IF_EQ jump offset (to alternate)
            if_eq_pos = len(bytecode)
            bytecode.extend(struct.pack("<B", OP_IF_EQ))
            bytecode.extend(struct.pack("<i", 0))  # Dummy offset, patched later

            # Compile consequent
            self.compile_expression(node.consequent, bytecode)

            # Placeholder for OP_JUMP to skip alternate
            jump_pos = len(bytecode)
            bytecode.extend(struct.pack("<B", OP_JUMP))
            bytecode.extend(struct.pack("<i", 0))  # Dummy offset, patched later

            # Compile alternate (after OP_IF_EQ jump target)
            alternate_start = len(bytecode)
            self.compile_expression(node.alternate, bytecode)
            alternate_end = len(bytecode)

            # Patch OP_IF_EQ jump offset (to alternate)
            if_eq_offset = alternate_start - (if_eq_pos + 5)  # 5 = OP_IF_EQ (1) + int (4)
            bytecode[if_eq_pos + 1:if_eq_pos + 5] = struct.pack("<i", if_eq_offset)

            # Patch OP_JUMP offset (to after alternate)
            jump_offset = alternate_end - (jump_pos + 5)  # 5 = OP_JUMP (1) + int (4)
            bytecode[jump_pos + 1:jump_pos + 5] = struct.pack("<i", jump_offset)
        else:
            raise EspgCompilerError(f"Unsupported expression type: {node.type}")

    def compile_node(self, node, bytecode):
        try:
            if node.type == "CallExpression":
                if node.callee.name == "moveSprite" and len(node.arguments) == 3:
                    for arg in node.arguments:
                        self.compile_expression(arg, bytecode)
                    bytecode.extend(struct.pack("<B", OP_MOVE_SPRITE))
                elif node.callee.name == "setAnimation" and len(node.arguments) == 2:
                    for arg in node.arguments:
                        self.compile_expression(arg, bytecode)
                    bytecode.extend(struct.pack("<B", OP_SET_ANIMATION))
                elif node.callee.name == "onTouchPress" and len(node.arguments) == 4:
                    rel_x = float(node.arguments[0].value)
                    rel_y = float(node.arguments[1].value)
                    rel_radius = float(node.arguments[2].value)
                    handler_func = node.arguments[3]
                    if not (0 <= rel_x <= 1 and 0 <= rel_y <= 1 and 0 <= rel_radius <= 1):
                        raise EspgCompilerError("Touch coordinates and radius must be between 0 and 1")
                    x_uint16 = int(rel_x * 65535)
                    y_uint16 = int(rel_y * 65535)
                    radius_uint16 = int(rel_radius * 65535)
                    handler_offset = len(bytecode) + 10
                    bytecode.extend(struct.pack("<BBBHHHi", OP_ON_EVENT, EVENT_TOUCH_PRESS, 4, x_uint16, y_uint16, radius_uint16, handler_offset))
                    self.compile_node(handler_func.body, bytecode)
                    bytecode.append(OP_RETURN)
                elif node.callee.name == "drawPixel" and len(node.arguments) == 3:
                    for arg in node.arguments:
                        self.compile_expression(arg, bytecode)
                    bytecode.extend(struct.pack("<B", OP_DRAW_PIXEL))
                elif node.callee.name == "drawLine" and len(node.arguments) == 5:
                    for arg in node.arguments:
                        self.compile_expression(arg, bytecode)
                    bytecode.extend(struct.pack("<B", OP_DRAW_LINE))
                elif node.callee.name == "drawRect" and len(node.arguments) >= 5:
                    for arg in node.arguments[:5]:
                        self.compile_expression(arg, bytecode)
                    filled = node.arguments[5].value if len(node.arguments) > 5 else False
                    bytecode.extend(struct.pack("<B", OP_LOAD_INT))
                    bytecode.extend(struct.pack("<i", 1 if filled else 0))
                    bytecode.extend(struct.pack("<B", OP_DRAW_RECT))
            elif node.type == "AssignmentExpression" and node.operator == "=":
                if node.left.type != "Identifier":
                    raise EspgCompilerError("Assignment target must be a variable")
                var_name = node.left.name
                if var_name not in self.variable_table:
                    raise EspgCompilerError(f"Undefined variable: {var_name}")
                var_index = self.variable_table[var_name]
                self.compile_expression(node.right, bytecode)
                bytecode.extend(struct.pack("<B", OP_STORE_VAR))
                bytecode.extend(struct.pack("<B", var_index))
            elif node.type == "BlockStatement":
                for stmt in node.body:
                    self.compile_node(stmt, bytecode)
            elif node.type == "ExpressionStatement":
                self.compile_node(node.expression, bytecode)
            elif node.type == "FunctionDeclaration" and node.id.name == "update":
                self.compile_node(node.body, bytecode)
            else:
                raise EspgCompilerError(f"Unsupported node type: {node.type}")
        except Exception as e:
            raise EspgCompilerError(f"Error compiling node {node.type}: {e}")

    def compile_game_logic(self) -> Tuple[bytes, List[Tuple[int, int, int, int]]]:
        bytecode = bytearray()
        event_table = []

        for node in self.config["game_logic_ast"]:
            self.compile_node(node, bytecode)

        bytecode.append(OP_END)
        return heatshrink2.encode(bytecode) if self.compress_logic else bytecode, event_table

    def assemble_espg(self):
        try:
            assets, compressed_assets = self.process_assets()
            logic_bytecode, event_table = self.compile_game_logic()

            asset_table_offset = HEADER_SIZE
            event_table_offset = asset_table_offset + (len(assets) * ASSET_ENTRY_SIZE)
            logic_offset = event_table_offset + (len(event_table) * EVENT_ENTRY_SIZE)

            with open(self.output_file, "wb") as f:
                header = struct.pack("<4sBHIHHI", MAGIC_NUMBER, VERSION, len(assets), asset_table_offset, len(event_table), event_table_offset, logic_offset)
                f.write(header)
                for entry in assets:
                    f.write(entry)
                for event_type, x_uint16, y_uint16, radius_uint16, offset in event_table:
                    f.write(struct.pack("<BHHHiB", event_type, x_uint16, y_uint16, radius_uint16, offset, 0))
                f.write(compressed_assets)
                f.write(logic_bytecode)
        except Exception as e:
            raise EspgCompilerError(f"Error assembling ESPG file: {e}")

    def compile(self):
        self.assemble_espg()
        print(f"Compiled '{self.output_file}' successfully")

if __name__ == "__main__":
    try:
        compiler = EspgCompiler("/home/spooky/Desktop/GameAPI/Examples/Pacman/Config.js")
        compiler.compile()
    except EspgCompilerError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
