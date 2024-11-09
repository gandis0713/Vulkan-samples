struct VertexOutput {
  @builtin(position) Position : vec4f,
  @location(0) fragColor : vec3f,
}

@vertex
fn main(
  @location(0) position : vec3f,
  @location(1) color : vec3f
) -> VertexOutput {
  var output : VertexOutput;
  output.Position = vec4(position, 1.0);
  output.fragColor = color;
  return output;
}