# Noto Fonts for NanoVG

This directory contains Noto fonts compiled from source and included in the project for comprehensive international text rendering support.

## Font Sources

All fonts are from Google's Noto font project, compiled from `/opt/` directories:

- **noto-emoji**: `/opt/noto-emoji/`
- **noto-cjk**: `/opt/noto-cjk/`
- **notofonts.github.io**: `/opt/notofonts.github.io/`

## Included Fonts

### Emoji (15 MB)
- `emoji/Noto-COLRv1.ttf` - **Vector color emoji** using COLR v1 table
  - Full color vector graphics for all emoji
  - Scalable to any size without quality loss
  - Modern OpenType standard
- `emoji/NotoColorEmoji.ttf` - **Bitmap color emoji** using CBDT table
  - Fallback for systems without COLR v1 support
  - Pre-rendered at fixed sizes

### CJK - Chinese, Japanese, Korean (48 MB)
- `cjk/NotoSansCJKsc-Regular.otf` - Simplified Chinese
- `cjk/NotoSansCJKjp-Regular.otf` - Japanese
- `cjk/NotoSansCJKkr-Regular.otf` - Korean

Each CJK font contains tens of thousands of glyphs covering the full Unicode CJK Unified Ideographs range.

### Latin/Sans-Serif (1.6 MB)
- `sans/NotoSans-Regular.ttf` - Regular weight
- `sans/NotoSans-Bold.ttf` - Bold weight

## Build Information

Fonts were compiled from source using the Noto build systems:

### Emoji Fonts
Built with `/opt/noto-emoji/full_rebuild.sh`:
- Uses Python build system with fontTools and Cairo
- Generates CBDT (bitmap) and COLR v1 (vector) variants
- Includes comprehensive emoji coverage per Unicode standard

### CJK Fonts
Pre-built OTF files from `/opt/noto-cjk/`:
- Adobe OpenType format
- Complete CJK Unified Ideographs coverage
- Optimized for on-screen rendering

### Sans Fonts
Built via `/opt/notofonts.github.io/` Google Fonts repository:
- TrueType format
- Variable font support available
- Latin, Latin Extended, and Greek coverage

## Usage in Code

### Loading Emoji Font
```c
// Load COLR v1 vector emoji
FT_Face face;
FT_New_Face(library, "fonts/emoji/Noto-COLRv1.ttf", 0, &face);
```

### Loading CJK Font
```c
// Load Japanese CJK
FT_Face face;
FT_New_Face(library, "fonts/cjk/NotoSansCJKjp-Regular.otf", 0, &face);
```

### Loading Sans Font
```c
// Load Latin sans-serif
FT_Face face;
FT_New_Face(library, "fonts/sans/NotoSans-Regular.ttf", 0, &face);
```

## Font Fallback Chain

For comprehensive international text support, use this fallback order:

1. **NotoSans-Regular.ttf** - Latin, Greek, Cyrillic
2. **NotoSansCJKjp-Regular.otf** - CJK ideographs
3. **Noto-COLRv1.ttf** - Color emoji (vector)
4. **NotoColorEmoji.ttf** - Color emoji (bitmap fallback)

## Licenses

All Noto fonts are licensed under the **SIL Open Font License 1.1**:
- Free to use, modify, and distribute
- Can be bundled with software
- No attribution required in UI
- Full license text: https://scripts.sil.org/OFL

## Font Characteristics

### File Sizes
| Category | Files | Total Size |
|----------|-------|------------|
| Emoji    | 2     | 15 MB      |
| CJK      | 3     | 48 MB      |
| Sans     | 2     | 1.6 MB     |
| **Total**| **7** | **~65 MB** |

### Glyph Coverage
| Font | Glyphs | Coverage |
|------|--------|----------|
| Noto-COLRv1.ttf | 3,664 | All Unicode emoji |
| NotoSansCJKjp-Regular.otf | 65,535 | CJK Unified Ideographs |
| NotoSans-Regular.ttf | 3,166 | Latin Extended, Greek, Cyrillic |

## Updating Fonts

To update fonts from source:

```bash
# Update emoji fonts
cd /opt/noto-emoji
./full_rebuild.sh
cp fonts/Noto-COLRv1.ttf /work/java/ai-ide-jvm/nanovg/fonts/emoji/

# CJK fonts are pre-built, just copy newer versions
cp /opt/noto-cjk/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf \
   /work/java/ai-ide-jvm/nanovg/fonts/cjk/

# Sans fonts
cp /opt/notofonts.github.io/fonts/NotoSans/googlefonts/ttf/NotoSans-Regular.ttf \
   /work/java/ai-ide-jvm/nanovg/fonts/sans/
```

## Technical Details

### COLR v1 vs CBDT
- **COLR v1** (Color Layer): Vector format, scalable, modern
  - Requires Cairo or FreeType 2.13+ with rendering support
  - Used by: Chrome, Firefox, Windows 11
- **CBDT** (Color Bitmap Data Table): Pre-rendered PNG images
  - Supported by all modern systems
  - Fixed sizes (usually 128x128)
  - Used by: Android, older systems

### CJK Font Selection
We include one font per script to balance coverage vs file size:
- **Simplified Chinese (SC)**: Used in mainland China
- **Japanese (JP)**: Includes kana + kanji
- **Korean (KR)**: Includes hangul + hanja

For Traditional Chinese, Simplified Chinese font provides reasonable coverage.

## References

- Noto Emoji: https://github.com/googlefonts/noto-emoji
- Noto CJK: https://github.com/notofonts/noto-cjk
- Noto Fonts: https://fonts.google.com/noto
- COLR v1 Spec: https://github.com/googlefonts/colr-gradients-spec
