BRIGHTEST_STARS_FILE = "brightest_stars.txt"
STARS_TEMPLATE_FILE = "Stars.h.template"
STARS_FILE = "Stars.h"
NAME_INDEX = 3
RA_INDEX = 5
DEC_INDEX = 6


def parse_ra(ra: str):
    return [float(element.strip('hms').replace(',','.')) for element in ra.split()]

def parse_dec(dec: str):
    d, m, s = dec.split()
    dec_split = [d[:-2], m[:-3], s[:-3]]
    return [float(e.replace(',','.')) for e in dec_split]

def parse_name_to_symbol(name: str):
    return ''.join(char if char.isalnum() else "_" for char in name)[0:16]

def parse_to_star_array_entry(star: list):
    symbol, name, ra, dec = star
    ra_str = ", ".join(str(r) for r in ra)
    dec_str = ", ".join(str(d) for d in dec)
    return f"{{StarKey::{symbol}, \"{name}\", RA{{{ra_str}}}, Dec{{{dec_str}}}}}"


stars = []
with open(BRIGHTEST_STARS_FILE, "rb") as f:
    for l in f:
        line = l.replace(b'\xe2\x88\x92', b'-').decode("cp1250", errors="ignore").split('\t')
        stars.append(
            (
                parse_name_to_symbol(line[NAME_INDEX]),
                line[NAME_INDEX][0:16],
                parse_ra(line[RA_INDEX]),
                parse_dec(line[DEC_INDEX])
            )
        )
stars = sorted(stars, key=lambda s: s[1])
with open(STARS_TEMPLATE_FILE, "r") as f:
    content = f.read()

    stars_enum = '\t' + ',\n\t'.join(s[0] for s in stars) + ",\n\tlast=" + stars[-1][0]
    content = content.replace("{{STARS_ENUM_KEYS}}",stars_enum)

    stars_array = '\t' + ',\n\t'.join(parse_to_star_array_entry(s) for s in stars)
    content = content.replace("{{STARS_ARRAY}}", stars_array)
    with open(STARS_FILE, "w") as sf:
        sf.write(content)
