# PDF Viewer

Sheet music viewer using LVGL.

## Table of Contents

- [Overview](#overview)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [License](#license)
- [Contributing](#contributing)
- [Contact](#contact)

## Overview

This app lets you view sheet music in a variety of formats:

- PDF
- SVG
- PNG
- JPEG
- MusicXML
- MIDI

You can also export your sheet music as a PDF.
For PDF files, you can view, edit, and save annotations.
Page turning is supported using a footswitch.

## Build

```
autoreconf -i --force
./configure
make
```

## Usage

```
./pdf_viewer_lvgl
```

## License

This project is licensed under the AGPL-3 License. See the [LICENSE](./LICENSE) file for details.

### External Tools

This project uses the following external tools and libraries:

1. [LVGL](https://lvgl.io/): Licensed under the MIT License.
2. [Verovio](https://www.verovio.org/): Licensed under the GNU Lesser General Public License (LGPL-3.0). Verovio is executed as a separate process and is not dynamically or statically linked with this software.
3. [Cairo](https://www.cairographics.org/): Licensed under the LGPL-2.1 License.
4. [MuPDF](https://mupdf.com/): Licensed under the AGPL-3 License. This project fully complies with the AGPL by ensuring any modifications and relevant usage of MuPDF are accompanied by source code disclosures.
5. [librsvg](https://gitlab.gnome.org/GNOME/librsvg): Licensed under the LGPL-2.1 License.
6. [cJSON](https://github.com/DaveGamble/cJSON): Licensed under the MIT License.


## Contributing

Contributions are welcome! Follow these steps to contribute:

1. Fork the repository and clone it to your local machine.
2. Create a new branch.
3. Make your changes and submit a pull request.

For more details, see CONTRIBUTING.md.

## Contact

Email: mushoku@msn.com

GitHub: https://github.com/tyano463
