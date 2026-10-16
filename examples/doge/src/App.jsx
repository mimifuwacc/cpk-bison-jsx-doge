import { useEffect, useState } from "react";
import { createRoot } from "react-dom/client";

function parseBmp(buffer) {
  const view = new DataView(buffer);
  if (view.getUint16(0, true) !== 0x4d42) {
    throw new Error("Unsupported BMP signature");
  }

  const pixelsOffset = view.getUint32(10, true);
  const width = view.getInt32(18, true);
  const rawHeight = view.getInt32(22, true);
  const height = Math.abs(rawHeight);
  const rowStride = Math.ceil((width * 3) / 4) * 4;
  const bytes = new Uint8Array(buffer);
  const pixels = [];

  for (let y = 0; y < height; y += 1) {
    const sourceY = rawHeight > 0 ? height - 1 - y : y;
    const rowStart = pixelsOffset + sourceY * rowStride;

    for (let x = 0; x < width; x += 1) {
      const offset = rowStart + x * 3;
      const color = `#${bytes[offset + 2].toString(16).padStart(2, "0")}${bytes[
        offset + 1
      ]
        .toString(16)
        .padStart(2, "0")}${bytes[offset].toString(16).padStart(2, "0")}`;
      pixels.push([x, y, color]);
    }
  }

  return { width, height, pixels };
}

function Doge() {
  const [dogeImage, setDogeImage] = useState(null);

  useEffect(() => {
    fetch("/doge.bmp")
      .then((response) => response.arrayBuffer())
      .then((buffer) => setDogeImage(parseBmp(buffer)));
  }, []);

  return (
    dogeImage && (
      <svg
        viewBox={`0 0 ${dogeImage.width} ${dogeImage.height}`}
        role="img"
        aria-label="犬 DOGE"
        shapeRendering="crispEdges"
        style={{
          width: "100%",
          height: "100%",
          display: "block",
          imageRendering: "pixelated",
        }}
      >
        <g>
          {dogeImage.pixels.map(([x, y, color]) => (
            <rect
              key={`${x}-${y}`}
              x={x}
              y={y}
              width="1"
              height="1"
              fill={color}
            />
          ))}
        </g>
      </svg>
    )
  );
}

function App() {
  return (
    <main
      style={{
        display: "grid",
        minHeight: "100vh",
        placeItems: "center",
      }}
    >
      <div
        style={{ width: "min(90vw, 90vh)", aspectRatio: "1" }}
        onClick={() => alert("ワン！")}
      >
        <Doge />
      </div>
    </main>
  );
}

createRoot(document.getElementById("root")).render(<App />);
