#ifndef GFIXED_H
#define GFIXED_H

typedef gint64 gfixed;
#define GINT_TO_FIXED(x)         ((gfixed) ((x) << 16))
#define GDOUBLE_TO_FIXED(x)      ((gfixed) ((x) * (1 << 16) + 0.5))
#define GFIXED_TO_INT(x)         ((x) >> 16)
#define GFIXED_TO_DOUBLE(x)      (((double) (x)) / (1 << 16))
#define GFIXED_ROUND_TO_INT(x)   (((x) + (1 << (16-1))) >> 16)
#define GFIXED_0                 0L
#define GFIXED_1                 65536L
#define GFIXED_2                 131072L
#define gfixed_mul(x, y)         (((x) * (y)) >> 16)
#define gfixed_div(x, y)         (((x) << 16) / (y))

#endif /* GFIXED_H */
