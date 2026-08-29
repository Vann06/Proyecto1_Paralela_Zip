# Matemática y lógica del rebote de las vacas dentro de una plataforma en forma de rombo

## 1. Idea general

La superficie verde donde se mueven las vacas tiene forma de **rombo**.  
En lugar de hacer que las vacas reboten contra los límites rectangulares de la ventana, se puede usar la ecuación matemática de un rombo para determinar si cada vaca continúa dentro de la plataforma.

La idea general es:

1. Mover cada vaca usando su velocidad.
2. Comprobar si su nueva posición sigue dentro del rombo.
3. Si salió del rombo, calcular qué dirección tiene el borde que tocó.
4. Reflejar el vector de velocidad respecto a ese borde.
5. Corregir ligeramente la posición para que la vaca vuelva a quedar dentro.

Esta lógica incorpora geometría, vectores y una aproximación física de reflexión.

---

## 2. Movimiento de cada vaca

Cada vaca tiene una posición:

$$
(x,y)
$$

y una velocidad:

$$
\vec{v}=(v_x,v_y)
$$

En cada frame se actualiza su posición con:

$$
x_{nuevo}=x+v_x\Delta t
$$

$$
y_{nuevo}=y+v_y\Delta t
$$

donde:

- $x,y$ son las coordenadas actuales.
- $v_x,v_y$ son las componentes de la velocidad.
- $\Delta t$ es el tiempo transcurrido entre un frame y el siguiente.

Esto permite que la velocidad sea independiente de los FPS.

---

## 3. Ecuación del rombo

Si el rombo está centrado en el origen $(0,0)$ y sus vértices son:

$$
(0,b), \qquad (a,0), \qquad (0,-b), \qquad (-a,0)
$$

entonces cualquier punto $(x,y)$ está dentro del rombo cuando:

$$
\boxed{\frac{|x|}{a}+\frac{|y|}{b}\leq1}
$$

donde:

- $a$ es la mitad del ancho del rombo.
- $b$ es la mitad de la altura del rombo.

Podemos definir:

$$
D=\frac{|x|}{a}+\frac{|y|}{b}
$$

Entonces:

$$
D<1 \Rightarrow \text{la vaca está dentro}
$$

$$
D=1 \Rightarrow \text{la vaca está sobre el borde}
$$

$$
D>1 \Rightarrow \text{la vaca salió del rombo}
$$

Esto reemplaza las comparaciones rectangulares del tipo:

```cpp
if (x > limite)
```

por una sola condición geométrica que funciona para los cuatro lados inclinados.

### 3.1. Distribución inicial

Para evitar que muchas vacas aparezcan proyectadas sobre el mismo borde, la implementación final ya no crea puntos en un rectángulo para después corregirlos. Se toman dos valores uniformes $u,v\in[0,1)$ y se aplica:

$$
x_n=u+v-1
$$

$$
y_n=u-v
$$

La transformación distribuye los puntos dentro del rombo unidad $|x_n|+|y_n|\leq1$. Después se escalan con `romboAncho`, `romboAlto` y un margen inicial de $0.78$. Por ello todas las vacas comienzan dentro de la superficie y ninguna nace pegada a un lado.

---

## 4. Identificación del borde

Los cuatro bordes del rombo son diagonales. Por eso no basta con invertir únicamente `vx` o `vy`.

La dirección perpendicular al borde se obtiene con un **vector normal**.

Para el rombo:

$$
n_x=\frac{\operatorname{sign}(x)}{a}
$$

$$
n_y=\frac{\operatorname{sign}(y)}{b}
$$

Por ejemplo:

- si $x>0$ y $y>0$, la vaca está en la zona superior derecha;
- si $x<0$ y $y>0$, está en la superior izquierda;
- si $x>0$ y $y<0$, está en la inferior derecha;
- si $x<0$ y $y<0$, está en la inferior izquierda.

El signo de las coordenadas permite escoger automáticamente la orientación de la normal correspondiente.

---

## 5. Normalización del vector

Antes de utilizar la normal para calcular el rebote, se convierte en un vector de longitud 1.

Su magnitud es:

$$
L=\sqrt{n_x^2+n_y^2}
$$

Luego:

$$
\hat{n}_x=\frac{n_x}{L}
$$

$$
\hat{n}_y=\frac{n_y}{L}
$$

Por lo tanto:

$$
\hat{n}=(\hat{n}_x,\hat{n}_y)
$$

es el vector normal unitario del borde.

---

## 6. Producto punto

La velocidad de la vaca es:

$$
\vec{v}=(v_x,v_y)
$$

El producto punto entre la velocidad y la normal se calcula como:

$$
\vec{v}\cdot\hat{n}
=
v_x\hat{n}_x+v_y\hat{n}_y
$$

Este valor indica qué parte de la velocidad está dirigida hacia el borde.

---

## 7. Reflexión de la velocidad

Para simular el rebote se utiliza la fórmula de reflexión vectorial:

$$
\boxed{
\vec{v}\,'=
\vec{v}
-
2(\vec{v}\cdot\hat{n})\hat{n}
}
$$

En componentes:

$$
v_x'=
v_x-
2(\vec{v}\cdot\hat{n})\hat{n}_x
$$

$$
v_y'=
v_y-
2(\vec{v}\cdot\hat{n})\hat{n}_y
$$

La nueva velocidad conserva aproximadamente la rapidez original, pero cambia su dirección según la inclinación del borde.

Esto produce un rebote más realista que simplemente hacer:

```cpp
vx = -vx;
```

o:

```cpp
vy = -vy;
```

porque las paredes de la plataforma son diagonales.

---

## 8. Corrección de posición

Debido al tiempo entre frames, una vaca puede avanzar ligeramente fuera del rombo antes de detectar la colisión.

Si:

$$
D=\frac{|x|}{a}+\frac{|y|}{b}>1
$$

podemos regresar el punto al borde mediante:

$$
x_{borde}=\frac{x}{D}
$$

$$
y_{borde}=\frac{y}{D}
$$

Esto funciona porque la ecuación del rombo es homogénea.

Para evitar que la vaca quede exactamente sobre el borde y vuelva a detectar otra colisión inmediatamente, la implementación introduce un pequeño factor:

$$
x=0.995\frac{x}{D}
$$

$$
y=0.995\frac{y}{D}
$$

De esta forma la vaca queda un poco dentro de la plataforma. Como la normal utilizada apunta hacia afuera, la reflexión se aplica solamente cuando:

$$
\vec{v}\cdot\hat{n}>0
$$

Si el producto es menor o igual que cero, la vaca ya se dirige hacia el interior y no debe reflejarse otra vez. Esta condición, junto con el margen de posición, evita la vibración en los lados. En un vértice, si una coordenada vale exactamente cero, se usa el signo de la componente correspondiente de la velocidad para seleccionar uno de los dos lados adyacentes y evitar que la vaca quede estancada.

---

## 9. Algoritmo completo

Para cada vaca:

```text
Actualizar rotación
        |
        v
Actualizar posición
x = x + vx * dt
y = y + vy * dt
        |
        v
Calcular
D = |x|/a + |y|/b
        |
        v
    ¿D > 1?
      /   \
    no     sí
    |       |
    |       v
    |   calcular normal
    |       |
    |       v
    |   normalizar
    |       |
    |       v
    |   producto punto
    |       |
    |       v
    |   reflejar velocidad
    |       |
    |       v
    |   corregir posición
    |       |
    +-------+
        |
        v
Siguiente vaca
```

---

## 10. Pseudocódigo

```text
para cada vaca:

    x = x + vx * dt
    y = y + vy * dt

    D = abs(x) / a + abs(y) / b

    si D > 1:

        nx = signo(x) / a
        ny = signo(y) / b

        longitud = sqrt(nx*nx + ny*ny)

        nx = nx / longitud
        ny = ny / longitud

        producto = vx*nx + vy*ny

        si producto > 0:
            vx = vx - 2*producto*nx
            vy = vy - 2*producto*ny

        x = 0.995 * x / D
        y = 0.995 * y / D
```

---

## 11. Relación con la paralelización

Esta operación se realiza de forma independiente para cada vaca.

La vaca $i$ solamente modifica:

- su posición;
- su velocidad;
- su rotación.

No necesita modificar los datos de otra vaca.

Por eso el ciclo:

```cpp
for (cada vaca) {
    mover();
    detectarColision();
    calcularRebote();
}
```

es un candidato natural para ser paralelizado posteriormente con OpenMP:

```cpp
#pragma omp parallel for
```

En la versión secuencial todas las vacas se actualizan una después de otra.  
En la versión paralela diferentes iteraciones pueden ser procesadas por varios hilos.

---

## 12. Fórmulas principales utilizadas

### Movimiento

$$
\boxed{x_{t+\Delta t}=x_t+v_x\Delta t}
$$

$$
\boxed{y_{t+\Delta t}=y_t+v_y\Delta t}
$$

### Detección dentro del rombo

$$
\boxed{
\frac{|x|}{a}+\frac{|y|}{b}\leq1
}
$$

### Normalización

$$
\boxed{
\hat{n}=\frac{\vec{n}}{\|\vec{n}\|}
}
$$

### Producto punto

$$
\boxed{
\vec{v}\cdot\hat{n}
=
v_x\hat{n}_x+v_y\hat{n}_y
}
$$

### Reflexión

$$
\boxed{
\vec{v}\,'=
\vec{v}
-
2(\vec{v}\cdot\hat{n})\hat{n}
}
$$

---

## 13. Conclusión

La plataforma en forma de rombo permite incorporar una lógica matemática directamente relacionada con el movimiento del screensaver. La ecuación del rombo determina cuándo ocurre una colisión y la reflexión vectorial calcula la nueva dirección de movimiento.

Además de mejorar visualmente el comportamiento de las vacas, esta solución es apropiada para el proyecto de computación paralela porque el cálculo se repite de manera independiente para cada elemento y posteriormente puede distribuirse entre varios hilos utilizando OpenMP.
