for f in *.ppm; do convert "$f" "${f%.ppm}.png"; done
