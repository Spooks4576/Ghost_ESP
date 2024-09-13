import argparse
import os

class BinFile:
    def __init__(self, file_path, addr):
        self.file_path = file_path
        self.file_name = os.path.basename(file_path)
        try:
            self.addr = int(addr, 16)  # Convert hexadecimal address to integer
        except ValueError:
            raise ValueError(f"Address must be a hexadecimal string, got {addr} instead.")
        self.size = os.path.getsize(self.file_path)

class MultipleBin:
    def __init__(self, name, output_folder):
        self.name = name
        self.output_folder = output_folder
        os.makedirs(self.output_folder, exist_ok=True)
        self.output_path = os.path.join(self.output_folder, self.name)
        self.bin_files = []

    def add_bin_file(self, file_path, addr):
        self.bin_files.append(BinFile(file_path, addr))

    def create_combined_bin(self):
        self.bin_files.sort(key=lambda x: x.addr)
        with open(self.output_path, "wb") as out_file:
            current_address = 0
            for bin_file in self.bin_files:
                gap = bin_file.addr - current_address
                if gap < 0:
                    raise ValueError(f"Overlapping detected with {bin_file.file_name}")
                out_file.write(b'\xFF' * gap)
                with open(bin_file.file_path, "rb") as f:
                    out_file.write(f.read())
                current_address = bin_file.addr + bin_file.size

def main():
    parser = argparse.ArgumentParser(description='Merge binary files at specified memory addresses.')
    parser.add_argument('--bin_path', action='append', required=True, help='Paths to binary files.')
    parser.add_argument('--bin_address', action='append', type=str, required=True, help='Memory addresses for each binary file.')
    
    args = parser.parse_args()

    if len(args.bin_path) != len(args.bin_address):
        print("Error: The number of paths and addresses must match.")
        return

    combined_bin = MultipleBin("combined_firmware.bin", "output")
    
    for path, addr in zip(args.bin_path, args.bin_address):
        combined_bin.add_bin_file(path, addr)

    combined_bin.create_combined_bin()
    print(f"Combined binary created successfully at {combined_bin.output_path}")

if __name__ == "__main__":
    main()
