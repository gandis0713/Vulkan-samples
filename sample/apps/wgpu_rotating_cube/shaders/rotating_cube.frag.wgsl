@fragment
fn main(
  @location(0) fragColor: vec3f
) -> @location(0) vec4f {
  return vec4(fragColor, 1.0);
}