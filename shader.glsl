#version 330 core
in vec2 fragCoord;
out vec4 FragColor;

uniform vec2 u_resolution;

uniform float u_centerx_hi;
uniform float u_centerx_lo;
uniform float u_centery_hi;
uniform float u_centery_lo;

uniform float u_zoom_hi;
uniform float u_zoom_lo;

uniform int u_maxIterations;

struct Double {
  float hi;  // старшие биты
  float lo;  // младшие биты
};

struct Dvec2 {
  Double x;
  Double y;
};

// Создание Double из float
Double makeDouble(float x) {
  return Double(x, 0.0);
}

// Сложение Double
Double addDouble(Double a, Double b) {
  float s = a.hi + b.hi;
  float v = s - a.hi;
  float e = (a.hi - (s - v)) + (b.hi - v);
  float t = a.lo + b.lo + e;
  
  float hi = s + t;
  float lo = t - (hi - s);
  
  return Double(hi, lo);
}

// разложение float на две части, чтобы восстановить точное умножение
void split(in float a, out float hi, out float lo) {
    float c = 4097.0 * a; // 2^12+1, работает для 24-битной мантиссы float
    float big = c - (c - a);
    hi = big;
    lo = a - big;
}

Double twoProd(float a, float b) {
    float p = a * b;
    float a_hi, a_lo, b_hi, b_lo;
    split(a, a_hi, a_lo);
    split(b, b_hi, b_lo);
    float err = ((a_hi*b_hi - p) + a_hi*b_lo + a_lo*b_hi) + a_lo*b_lo;
    return Double(p, err);
}

Double mulDouble(Double a, Double b) {
    Double p = twoProd(a.hi, b.hi);        // точное умножение hi*hi
    float cross = a.hi*b.lo + a.lo*b.hi;   // перекрёстные
    float lo = p.lo + cross + a.lo*b.lo;
    float hi = p.hi + lo;
    lo = lo - (hi - p.hi);
    return Double(hi, lo);
}

// // Умножение Double
// Double mulDouble(Double a, Double b) {
//   float c11 = a.hi * b.hi;
//   float c21 = a.lo * b.hi;
//   float c12 = a.hi * b.lo;
//   float c22 = a.lo * b.lo;
//   
//   float t1 = c21 + c12;
//   float t2 = c11;
//   
//   float hi = t2 + t1;
//   float lo = t1 - (hi - t2) + c22;
//   
//   return Double(hi, lo);
// }

Double DDot(Dvec2 a, Dvec2 b){
  return addDouble(mulDouble(a.x, b.x), mulDouble(a.y, b.y));
}

Dvec2 makeDvec2(vec2 x){
  return Dvec2(makeDouble(x.x), makeDouble(x.y));
}

Dvec2 addDvec2(Dvec2 a, Dvec2 b) {
  return Dvec2(addDouble(a.x, b.x), addDouble(a.y, b.y));
}

// Конвертация в float
float doubleToFloat(Double d) {
  return d.hi + d.lo;
}

Double negateDouble(Double d) {
  // Если lo равно нулю, просто меняем знак hi
  if (d.lo == 0.0) {
    return Double(-d.hi, 0.0);
  }
  
  // Иначе меняем знаки обеих частей
  return Double(-d.hi, -d.lo);
}

Dvec2 mulDvec2byD(Dvec2 a, Double b) {
  return Dvec2(mulDouble(a.x, b), mulDouble(a.y, b));
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

bool greaterDouble(Double a, Double b) {
    if (a.hi > b.hi) return true;
    if (a.hi < b.hi) return false;
    return a.lo > b.lo;
}

int mandelbrot(Dvec2 c) {
  Dvec2 z = Dvec2(makeDouble(0.0), makeDouble(0.0));
  int iterations = 0;
  
  for (int i = 0; i < u_maxIterations; i++) {
    if (greaterDouble(DDot(z, z), makeDouble(4.0))) break;
    // z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
    z = Dvec2(addDouble(mulDouble(z.x, z.x), negateDouble(mulDouble(z.y, z.y))), mulDouble(mulDouble(makeDouble(2.0), z.x), z.y));
    z = Dvec2(addDouble(c.x, z.x), addDouble(c.y, z.y));
    iterations++;
  }
  
  return iterations;
}

void main() {
  // Преобразование координат экрана в комплексную плоскость
  vec2 uv = (fragCoord * 0.5 + 0.5) * u_resolution;
  Dvec2 c = addDvec2(mulDvec2byD(makeDvec2((uv - u_resolution * 0.5) / min(u_resolution.x, u_resolution.y)), Double(u_zoom_hi, u_zoom_lo)), Dvec2(Double(u_centerx_hi, u_centerx_lo), Double(u_centery_hi, u_centery_lo)));
  
  int iterations = mandelbrot(c);
  
  if (iterations == u_maxIterations) {
    // Точка принадлежит множеству - черный цвет
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  } else {
    // Цветовая схема на основе количества итераций
    float t = float(iterations) / float(u_maxIterations);
    
    // HSV цвета для красивого градиента
    vec3 color = hsv2rgb(vec3(t * 6.0 + 0.1, 0.8, 1.0));
    FragColor = vec4(color, 1.0);
  }
}
