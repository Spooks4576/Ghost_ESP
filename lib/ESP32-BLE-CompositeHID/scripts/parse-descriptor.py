import re
import sys

def reformat_text(input_text):
    # Regular expression to match lines with hexadecimal numbers
    pattern = re.compile(r'([0-9a-fA-F]+)\s*([0-9a-fA-F]+)?\s*(.*)$', re.MULTILINE)

    # Find all matches in the input text
    matches = pattern.findall(input_text)

    # Reformat each match
    reformatted_lines = []
    for match in matches:
        hex_numbers = [f'0x{num},' for num in filter(None, [match[0], match[1]])]  # Filter out None values

        comment_part = f' //{match[2].rstrip()}' if match[2] else ''  # Preserve whitespace in the comment

        reformatted_line = ' '.join(hex_numbers) + comment_part
        reformatted_lines.append(reformatted_line)

    # Join the reformatted lines into a single string
    output_text = '\n'.join(reformatted_lines)

    return output_text

if __name__ == "__main__":
    # Read input text from stdin
    input_text = sys.stdin.read()

    # Reformat the text
    output_text = reformat_text(input_text)

    # Print the reformatted text
    print(output_text)
