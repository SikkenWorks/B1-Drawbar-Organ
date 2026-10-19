from pathlib import Path
r=Path(__file__).resolve().parents[1]
usb=(r/'firmware/usb.c').read_text()
main=(r/'firmware/b1_main.c').read_text()
html=(r/'B1-WebInstaller.html').read_text()
assert '0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7' in usb
assert 'usb.uboot_req' in main and 'fm1_enter_uboot()' in main
assert 'UBOOT: [0xF0,0x22,0x24,0x35,0x7D,0xF7]' in html
assert 'id="enter-uboot"' in html and 'enterUboot()' in html
assert 'if(installing)' in html and 'isSupportedEtchIdentity(dev.id.text)' in html
assert 'Could not' not in main
print('USB UBOOT non-flashing handoff code audit: PASS (offline only)')
