export function bmpToSvg(buffer: Buffer): string {
  if (buffer.toString("ascii", 0, 2) !== "BM") {
    throw new Error("Unsupported BMP signature");
  }

  const pixelOffset = buffer.readUInt32LE(10);
  const dibSize = buffer.readUInt32LE(14);
  const width = buffer.readInt32LE(18);
  const rawHeight = buffer.readInt32LE(22);
  const bitsPerPixel = buffer.readUInt16LE(28);
  const compression = buffer.readUInt32LE(30);

  if (dibSize < 40 || bitsPerPixel !== 24 || compression !== 0) {
    throw new Error("Only uncompressed 24-bit BMP files are supported");
  }

  const height = Math.abs(rawHeight);
  const rowStride = Math.ceil((width * 3) / 4) * 4;
  const rects: string[] = [];

  for (let y = 0; y < height; y += 1) {
    const sourceY = rawHeight > 0 ? height - 1 - y : y;
    const rowStart = pixelOffset + sourceY * rowStride;
    let runStart = 0;
    let runColor = "";

    for (let x = 0; x <= width; x += 1) {
      let color = "";
      if (x < width) {
        const pixel = rowStart + x * 3;
        color = `#${buffer[pixel + 2].toString(16).padStart(2, "0")}${buffer[
          pixel + 1
        ]
          .toString(16)
          .padStart(2, "0")}${buffer[pixel].toString(16).padStart(2, "0")}`;
      }

      if (color !== runColor) {
        if (runColor) {
          rects.push(
            `<rect x="${runStart}" y="${y}" width="${
              x - runStart
            }" height="1" fill="${runColor}"/>`,
          );
        }
        runStart = x;
        runColor = color;
      }
    }
  }

  return [
    `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${width} ${height}" shape-rendering="crispEdges">`,
    "<g>",
    ...rects,
    "</g>",
    "</svg>",
    "",
  ].join("\n");
}
