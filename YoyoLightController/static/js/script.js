//Press Ctrl+F5 in browser to force reload javascript files
$(document).ready(function () {
  const TAU = 2 * Math.PI;

  const canvas = document.querySelector("#canvas");

  const NUM_SECTORS = 2;
  const NUM_PIXELS = 3;
  const NUM_COLORS = 3;
  const SECTOR_GAP = 1 - 0.1;
  const PIXEL_GAP = 1 - 0.1;

  //create a 3D array for circular matrix with color depth
  var pixel_states = new Array(NUM_SECTORS);
  for (let i = 0; i < NUM_SECTORS; i++) {
    pixel_states[i] = new Array(NUM_PIXELS);
    for (let j = 0; j < NUM_PIXELS; j++) {
      pixel_states[i][j] = new Array(NUM_COLORS);
    }
  }
  //initialize the array
  for (let i = 0; i < NUM_SECTORS; i++) {
    for (let j = 0; j < NUM_PIXELS; j++) {
      for (let k = 0; k < NUM_COLORS; k++) {
        pixel_states[i][j][k] = 0;
      }
    }
  }

  const HIGHLIGHT_COLOR = "rgba(0,200,0, .90)";
  const BASE_COLOR = "rgba(0,150,150, 1.0)";
  const CLICK_COLOR = "rgba(0,0,200, 1.0)";

  const CENTER_X = canvas.width / 2;
  const CENTER_Y = canvas.height / 2;
  const PIXEL_OFFSET = 1;
  var circle_radius = Math.min(canvas.width, canvas.height) / 2;
  circle_radius -= (circle_radius * PIXEL_OFFSET) / NUM_PIXELS;
  var pixel_width = (circle_radius * PIXEL_GAP) / NUM_PIXELS;

  var mousedown = false;

  if (canvas.getContext) {
    const ctx = canvas.getContext("2d");

    ctx.lineWidth = pixel_width;

    for (let i = 0; i < NUM_SECTORS; i++) {
      let arc_angle = (2 * Math.PI) / NUM_SECTORS;
      let start_angle = i * arc_angle;
      let end_angle = i * arc_angle + arc_angle * SECTOR_GAP;

      for (let j = 0; j < NUM_PIXELS; j++) {
        let arc_dist = (circle_radius * (j + PIXEL_OFFSET)) / NUM_PIXELS;

        ctx.strokeStyle = BASE_COLOR;

        ctx.beginPath();
        ctx.arc(CENTER_X, CENTER_Y, arc_dist, start_angle, end_angle);
        ctx.stroke();
      }
    }

    var last_pixel_coords;

    canvas.addEventListener("mousemove", function (e) {
      var cRect = canvas.getBoundingClientRect(); // Gets CSS pos, and width/height
      let mouseX = Math.round(e.clientX - cRect.left); // Subtract the 'left' of the canvas
      let mouseY = Math.round(e.clientY - cRect.top); // from the X/Y positions to make
      let polar_coords = rect_to_polar(mouseX, mouseY);
      let pixel_coords = polar_to_pixel(polar_coords[0], polar_coords[1]);

      if (last_pixel_coords == null) {
        update_pixel(ctx, pixel_coords[0], pixel_coords[1], HIGHLIGHT_COLOR);
      } else {
        if (
          pixel_coords[0] != last_pixel_coords[0] ||
          pixel_coords[1] != last_pixel_coords[1]
        ) {
          if (mousedown) {
            update_pixel(ctx, pixel_coords[0], pixel_coords[1], CLICK_COLOR);
          } else {
            update_pixel(
              ctx,
              pixel_coords[0],
              pixel_coords[1],
              HIGHLIGHT_COLOR
            );
            update_pixel(
              ctx,
              last_pixel_coords[0],
              last_pixel_coords[1],
              BASE_COLOR
            );
            console.log(
              "Sector: " + pixel_coords[0] + ", Pixel: " + pixel_coords[1]
            );
          }
        } else {
          if (mousedown) {
            update_pixel(ctx, pixel_coords[0], pixel_coords[1], CLICK_COLOR);
          }
        }
      }

      last_pixel_coords = pixel_coords;
    });

    canvas.addEventListener("mousedown", function (e) {
      console.log("Click");
      mousedown = true;
    });
    canvas.addEventListener("mouseup", function (e) {
      console.log("Up");
      mousedown = false;
    });
  }

  function update_pixel(ctx, sector, pixel, color) {
    if (pixel < NUM_PIXELS) {
      if (color == CLICK_COLOR) {
        pixel_states[sector][pixel][0] = 100;
      } else if (color == BASE_COLOR) {
        pixel_states[sector][pixel][0] = 0;
      }
      // arcs drawn cw
      // angle goes ccw
      let arc_angle = TAU / NUM_SECTORS;
      let start_angle = (NUM_SECTORS - sector - 1) * arc_angle; //invert the count so arc is drawn properly
      let end_angle = start_angle + arc_angle * SECTOR_GAP;
      let arc_dist = (circle_radius * (pixel + PIXEL_OFFSET)) / NUM_PIXELS;

      ctx.strokeStyle = color;
      ctx.beginPath();
      ctx.arc(CENTER_X, CENTER_Y, arc_dist, start_angle, end_angle);
      ctx.stroke();
    }
  }

  // input: x and y coord
  // output: [magnitude, positive angle in degrees]
  function rect_to_polar(x, y) {
    x = x - CENTER_X;
    y = -(y - CENTER_Y);
    let magnitude = Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2));
    let angle = Math.atan2(y, x);
    if (angle < 0) {
      angle = 2 * Math.PI + angle; //Converts neg angle to pos angle
    }
    angle = (angle * 180) / Math.PI; //Radians to Degrees
    // console.log(angle)
    return [magnitude, angle];
  }

  function polar_to_pixel(magnitude, angle) {
    let pixel = Math.round(
      ((magnitude - PIXEL_OFFSET * pixel_width) * NUM_PIXELS) / circle_radius
    );
    let sector = Math.floor((angle * NUM_SECTORS) / 360);
    return [sector, pixel];
  }
  $("#btn2").click(function () {
    data = JSON.stringify(pixel_states);
    $.post("/array", { canvas_data: data })
      .done(function (response) {
        console.log("Sucess: " + response);
        console.log(data);
      })
      .fail(function () {
        console.log("Fail");
      });
  });
});
