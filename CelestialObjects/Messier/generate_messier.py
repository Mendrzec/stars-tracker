MESSIER_OBJECTS_FILE = "messier_objects.txt"
MESSIER_TEMPLATE_FILE = "Messier.h.template"
MESSIER_FILE = "Messier.h"
NUMBER_INDEX = 0
NAME_INDEX = 2
RA_INDEX = 8
DEC_INDEX = 9


def parse_ra(ra: str):
    hms = ra.split()
    hms_split = [
        hms[0] if len(hms) > 0 else "0",
        hms[1] if len(hms) > 1 else "0",
        hms[2] if len(hms) > 2 else "0"
    ]
    return [float(element.strip('hms').replace(',','.')) for element in hms_split]

def parse_dec(dec: str):
    dms = dec.split()
    dec_split = [
        dms[0][:-2] if len(dms) > 0 else "0",
        dms[1][:-3] if len(dms) > 1 else "0",
        dms[2][:-3] if len(dms) > 2 else "0"
    ]
    return [float(e.replace(',','.')) for e in dec_split]

def parse_name_to_symbol(name: str):
    return ''.join(char if char.isalnum() else "_" for char in name)[0:16]

def parse_messier_number(number: str):
    return number.split('[')[0]

def parse_to_star_array_entry(star: list):
    symbol, name, ra, dec = star
    ra_str = ", ".join(str(r) for r in ra)
    dec_str = ", ".join(str(d) for d in dec)
    return f"MessierObject(Messier::{symbol}, \"{name}\", RA{{{ra_str}}}, Dec{{{dec_str}}})"


stars = []
with open(MESSIER_OBJECTS_FILE, "rb") as f:
    for l in f:
        line = l.replace(b'\xe2\x88\x92', b'-').replace(b'\xe2\x80\x93', b'').decode("cp1250", errors="ignore").split('\t')
        stars.append(
            (
                parse_messier_number(line[NUMBER_INDEX]) + "_" + parse_name_to_symbol(line[NAME_INDEX]),
                parse_messier_number(line[NUMBER_INDEX]) + " " + line[NAME_INDEX][0:16],
                parse_ra(line[RA_INDEX]),
                parse_dec(line[DEC_INDEX])
            )
        )

stars = sorted(stars, key=lambda s: s[1])
with open(MESSIER_TEMPLATE_FILE, "r") as f:
    content = f.read()

    stars_enum = '\t' + ',\n\t'.join(s[0] for s in stars) + ",\n\tlast=" + stars[-1][0]
    content = content.replace("{{MESSIER_ENUM_KEYS}}",stars_enum)

    stars_array = '\t' + ',\n\t'.join(parse_to_star_array_entry(s) for s in stars)
    content = content.replace("{{MESSIER_ARRAY}}", stars_array)
    with open(MESSIER_FILE, "w") as sf:
        sf.write(content)
