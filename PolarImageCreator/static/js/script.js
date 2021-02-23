//Press Ctrl+F5 in browser to force reload javascript files

const canvas = document.querySelector("#canvas");
const num_sectors = 20;
const num_pixels = 10;
//initialize a 2D array for circular matrix
var pixel_states = new Array(num_sectors);
for (let i = 0; i < pixel_states.length; i++) {
  pixel_states[i] = new Array(num_pixels).fill(false);
}

const center_x = canvas.width / 2;
const center_y = canvas.height / 2;
const pixel_offset = 1;
var circle_radius = Math.min(canvas.width, canvas.height) / 2;
circle_radius -= (circle_radius * pixel_offset) / num_pixels;

if (canvas.getContext) {
  const ctx = canvas.getContext("2d");

  ctx.fillStyle = "rgba(10,10,10,0.8)";
  ctx.lineWidth = (circle_radius * 0.9) / num_pixels;

  for (let i = 0; i < num_sectors; i++) {
    var sector_angle = (2 * Math.PI) / num_sectors;
    var start_angle = i * sector_angle;
    var end_angle = i * sector_angle + sector_angle * 0.95;

    for (let j = 0; j < num_pixels; j++) {
      var arc_radius = (circle_radius * (j + pixel_offset)) / num_pixels;
      if (pixel_states[i][j]) {
        ctx.strokeStyle = "rgba(0,200,0, 1.0)";
      } else {
        ctx.strokeStyle = "rgba(10,10,10,0.8)";
      }

      ctx.beginPath();
      ctx.arc(center_x, center_y, arc_radius, start_angle, end_angle);
      ctx.stroke();
    }
  }

  canvas.addEventListener("mousemove", function (e) {
    var cRect = canvas.getBoundingClientRect(); // Gets CSS pos, and width/height
    let mouseX = Math.round(e.clientX - cRect.left); // Subtract the 'left' of the canvas
    let mouseY = Math.round(e.clientY - cRect.top); // from the X/Y positions to make
    let polar_coords = rect_to_polar(mouseX, mouseY);
    let pixel_coords = polar_to_pixel(polar_coords[0], polar_coords[1]);
    pixel_states[pixel_coords[0] - 1][pixel_coords[1]] = true;
    ctx.clearRect(0, 0, 100, 200);
    ctx.fillText(
      "Sector: " + pixel_coords[0] + ", Pixel: " + pixel_coords[1],
      10,
      20
    );
  });
}

// input: x and y coord
// output: [magnitude, positive angle in degrees]
function rect_to_polar(x, y) {
  x = x - center_x;
  y = -(y - center_y);
  let magnitude = Math.sqrt(Math.pow(x, 2), Math.pow(y, 2));
  let angle = Math.atan2(y, x);
  if (angle < 0) {
    angle = 2 * Math.PI + angle; //Converts neg angle to pos angle
  }
  angle = (angle * 180) / Math.PI; //Radians to Degrees

  return [magnitude, angle];
}

function polar_to_pixel(radius, angle) {
  let pixel = Math.round(
    ((radius - (pixel_offset - 1) * arc_radius) * num_pixels) / circle_radius
  );
  let sector = Math.round((angle * num_sectors) / 360);
  return [sector, pixel];
}
