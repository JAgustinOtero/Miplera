"""
Analizador de Perfil Radial CAD
- Carga DXF
- Selecciona el centro manualmente o usa el centro del círculo de referencia
- Analiza el contorno en 360 grados
- Resolución: 0.5° a 2.0°, en pasos de 0.1°
- Exporta CSV con ángulo y distancia en mm

Dependencias:
    pip install ezdxf shapely numpy

Uso:
    python analizador_radial.py
"""

import math
import csv
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

try:
    import ezdxf
    from shapely.geometry import LineString, Point
except ImportError:
    raise SystemExit(
        "Faltan dependencias. Ejecutá:\n\n"
        "pip install ezdxf shapely numpy"
    )

try:
    import numpy as np
except ImportError:
    raise SystemExit("Falta numpy. Ejecutá: pip install numpy")


class RadialAnalyzer:
    def __init__(self, root):
        self.root = root
        self.root.title("Analizador de Perfil Radial CAD")
        self.root.geometry("900x650")
        self.doc = None
        self.entities = []
        self.center = None
        self.results = []

        self.build_ui()

    def build_ui(self):
        main = ttk.Frame(self.root, padding=12)
        main.pack(fill="both", expand=True)

        # Archivo
        file_frame = ttk.LabelFrame(main, text="Archivo CAD", padding=10)
        file_frame.pack(fill="x", pady=(0, 10))

        self.file_var = tk.StringVar(value="Ningún archivo seleccionado")
        ttk.Label(file_frame, textvariable=self.file_var).pack(
            side="left", fill="x", expand=True
        )
        ttk.Button(file_frame, text="Cargar DXF", command=self.load_dxf).pack(
            side="right"
        )

        # Centro
        center_frame = ttk.LabelFrame(main, text="Centro de análisis", padding=10)
        center_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(center_frame, text="X:").grid(row=0, column=0, padx=5)
        self.x_var = tk.StringVar(value="0")
        ttk.Entry(center_frame, textvariable=self.x_var, width=14).grid(
            row=0, column=1, padx=5
        )

        ttk.Label(center_frame, text="Y:").grid(row=0, column=2, padx=5)
        self.y_var = tk.StringVar(value="0")
        ttk.Entry(center_frame, textvariable=self.y_var, width=14).grid(
            row=0, column=3, padx=5
        )

        ttk.Button(
            center_frame,
            text="Detectar centro de CIRCLE",
            command=self.detect_circle_center
        ).grid(row=0, column=4, padx=15)

        ttk.Label(
            center_frame,
            text="También podés introducir X/Y manualmente."
        ).grid(row=1, column=0, columnspan=5, sticky="w", pady=(8, 0))

        # Configuración
        config = ttk.LabelFrame(main, text="Configuración", padding=10)
        config.pack(fill="x", pady=(0, 10))

        ttk.Label(config, text="Resolución angular:").grid(
            row=0, column=0, padx=5
        )

        values = [f"{x / 10:.1f}°" for x in range(5, 21)]
        self.res_var = tk.StringVar(value="1.0°")
        self.res_combo = ttk.Combobox(
            config,
            textvariable=self.res_var,
            values=values,
            state="readonly",
            width=10
        )
        self.res_combo.grid(row=0, column=1, padx=5)

        ttk.Label(config, text="Sentido:").grid(row=0, column=2, padx=(30, 5))

        self.direction_var = tk.StringVar(value="Horario")
        ttk.Radiobutton(
            config, text="Horario", variable=self.direction_var,
            value="Horario"
        ).grid(row=0, column=3, padx=5)
        ttk.Radiobutton(
            config, text="Antihorario", variable=self.direction_var,
            value="Antihorario"
        ).grid(row=0, column=4, padx=5)

        # Botones
        buttons = ttk.Frame(main)
        buttons.pack(fill="x", pady=(0, 10))

        ttk.Button(
            buttons, text="ANALIZAR 360°",
            command=self.analyze
        ).pack(side="left", padx=(0, 8))

        ttk.Button(
            buttons, text="Exportar CSV",
            command=self.export_csv
        ).pack(side="left", padx=8)

        # Tabla
        table_frame = ttk.LabelFrame(main, text="Resultados", padding=5)
        table_frame.pack(fill="both", expand=True)

        columns = ("angulo", "distancia")
        self.tree = ttk.Treeview(
            table_frame,
            columns=columns,
            show="headings"
        )
        self.tree.heading("angulo", text="Ángulo (°)")
        self.tree.heading("distancia", text="Distancia (mm)")
        self.tree.column("angulo", width=150, anchor="center")
        self.tree.column("distancia", width=180, anchor="center")

        scrollbar = ttk.Scrollbar(
            table_frame, orient="vertical", command=self.tree.yview
        )
        self.tree.configure(yscrollcommand=scrollbar.set)

        self.tree.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        self.status_var = tk.StringVar(value="Listo.")
        ttk.Label(
            main, textvariable=self.status_var, relief="sunken", anchor="w"
        ).pack(fill="x", pady=(8, 0))

    def load_dxf(self):
        path = filedialog.askopenfilename(
            title="Seleccionar archivo DXF",
            filetypes=[("DXF", "*.dxf"), ("Todos los archivos", "*.*")]
        )

        if not path:
            return

        try:
            self.doc = ezdxf.readfile(path)
            self.entities = list(self.doc.modelspace())
            self.file_var.set(path)
            self.status_var.set(
                f"DXF cargado: {len(self.entities)} entidades encontradas."
            )
            messagebox.showinfo(
                "DXF cargado",
                f"Archivo cargado correctamente.\n\n"
                f"Entidades encontradas: {len(self.entities)}"
            )
        except Exception as e:
            messagebox.showerror(
                "Error",
                f"No se pudo cargar el DXF:\n\n{e}"
            )

    def detect_circle_center(self):
        if not self.entities:
            messagebox.showwarning(
                "Atención",
                "Primero cargá un archivo DXF."
            )
            return

        circles = [
            e for e in self.entities
            if e.dxftype() == "CIRCLE"
        ]

        if not circles:
            messagebox.showwarning(
                "Sin círculos",
                "No se encontró ninguna entidad CIRCLE en el DXF."
            )
            return

        if len(circles) == 1:
            c = circles[0]
            self.center = (float(c.dxf.center.x), float(c.dxf.center.y))
        else:
            # Preferimos el círculo de mayor radio como círculo exterior.
            c = max(circles, key=lambda e: float(e.dxf.radius))
            self.center = (float(c.dxf.center.x), float(c.dxf.center.y))

        self.x_var.set(f"{self.center[0]:.6f}")
        self.y_var.set(f"{self.center[1]:.6f}")

        self.status_var.set(
            f"Centro detectado: X={self.center[0]:.6f}, "
            f"Y={self.center[1]:.6f}"
        )

    def get_center(self):
        try:
            return float(self.x_var.get()), float(self.y_var.get())
        except ValueError:
            raise ValueError("Las coordenadas X/Y del centro no son válidas.")

    def entity_to_segments(self, entity):
        """Convierte entidades DXF comunes en segmentos lineales."""

        typ = entity.dxftype()

        if typ == "LINE":
            p1 = entity.dxf.start
            p2 = entity.dxf.end
            return [(float(p1.x), float(p1.y)),
                    (float(p2.x), float(p2.y))]

        if typ == "LWPOLYLINE":
            points = list(entity.get_points("xy"))
            closed = bool(entity.closed)
            if len(points) < 2:
                return []

            result = []
            for i in range(len(points) - 1):
                result.extend([
                    (float(points[i][0]), float(points[i][1])),
                    (float(points[i + 1][0]), float(points[i + 1][1]))
                ])

            if closed:
                result.extend([
                    (float(points[-1][0]), float(points[-1][1])),
                    (float(points[0][0]), float(points[0][1]))
                ])
            return result

        if typ == "POLYLINE":
            verts = list(entity.vertices)
            if len(verts) < 2:
                return []

            result = []
            pts = [
                (float(v.dxf.location.x), float(v.dxf.location.y))
                for v in verts
            ]

            for i in range(len(pts) - 1):
                result.extend([pts[i], pts[i + 1]])

            if entity.is_closed:
                result.extend([pts[-1], pts[0]])

            return result

        # Para ARC/CIRCLE se genera una poligonal suficientemente fina.
        if typ in ("ARC", "CIRCLE"):
            center = entity.dxf.center
            cx = float(center.x)
            cy = float(center.y)
            radius = float(entity.dxf.radius)

            if typ == "CIRCLE":
                a0 = 0.0
                a1 = 360.0
            else:
                a0 = float(entity.dxf.start_angle)
                a1 = float(entity.dxf.end_angle)
                if a1 <= a0:
                    a1 += 360.0

            # Segmentación aproximada de 0.25 grados.
            n = max(8, int(math.ceil((a1 - a0) / 0.25)))
            angles = np.linspace(
                math.radians(a0),
                math.radians(a1),
                n + 1
            )

            pts = [
                (cx + radius * math.cos(a),
                 cy + radius * math.sin(a))
                for a in angles
            ]

            result = []
            for i in range(len(pts) - 1):
                result.extend([pts[i], pts[i + 1]])

            return result

        return []

    def build_geometry(self):
        """Construye una lista de segmentos de la geometría."""
        segments = []

        for entity in self.entities:
            # No usamos el círculo de referencia como contorno.
            # El análisis se realiza contra la geometría completa.
            segs = self.entity_to_segments(entity)

            for i in range(0, len(segs), 2):
                if i + 1 < len(segs):
                    segments.append((segs[i], segs[i + 1]))

        return segments

    @staticmethod
    def ray_segment_intersection(cx, cy, dx, dy, p1, p2):
        """
        Intersección entre:
            P = centro + t * dirección, t >= 0
        y:
            Q = p1 + u * (p2-p1), 0 <= u <= 1

        Devuelve t (distancia) o None.
        """
        x1, y1 = p1
        x2, y2 = p2

        sx = x2 - x1
        sy = y2 - y1

        cross = dx * sy - dy * sx

        if abs(cross) < 1e-12:
            return None

        qpx = x1 - cx
        qpy = y1 - cy

        t = (qpx * sy - qpy * sx) / cross
        u = (qpx * dy - qpy * dx) / cross

        if t >= 0 and 0 <= u <= 1:
            return t

        return None

    def analyze(self):
        if not self.entities:
            messagebox.showwarning(
                "Atención",
                "Primero cargá un archivo DXF."
            )
            return

        try:
            cx, cy = self.get_center()
        except ValueError as e:
            messagebox.showerror("Error", str(e))
            return

        try:
            resolution = float(self.res_var.get().replace("°", ""))
        except ValueError:
            messagebox.showerror("Error", "Resolución angular inválida.")
            return

        segments = self.build_geometry()

        if not segments:
            messagebox.showerror(
                "Error",
                "No se encontró geometría compatible para analizar."
            )
            return

        # Generamos exactamente 360/resolución posiciones.
        count = int(round(360.0 / resolution))
        angles = [i * resolution for i in range(count)]

        if self.direction_var.get() == "Antihorario":
            angles = list(reversed(angles))

        self.results = []

        for angle_deg in angles:
            angle_rad = math.radians(angle_deg)
            dx = math.cos(angle_rad)
            dy = math.sin(angle_rad)

            distances = []

            for p1, p2 in segments:
                d = self.ray_segment_intersection(
                    cx, cy, dx, dy, p1, p2
                )
                if d is not None:
                    distances.append(d)

            if distances:
                # Para una figura inscripta, tomamos la intersección
                # más lejana: corresponde al perfil exterior visto
                # desde el centro.
                distance = min(distances)
            else:
                distance = None

            self.results.append((angle_deg, distance))

        # Actualizar tabla.
        for item in self.tree.get_children():
            self.tree.delete(item)

        valid = 0

        for angle, distance in self.results:
            if distance is None:
                value = ""
            else:
                value = f"{distance:.4f}"
                valid += 1

            self.tree.insert(
                "",
                "end",
                values=(f"{angle:.1f}", value)
            )

        self.status_var.set(
            f"Análisis terminado: {len(self.results)} ángulos, "
            f"{valid} mediciones válidas."
        )

        messagebox.showinfo(
            "Análisis terminado",
            f"Se analizaron {len(self.results)} posiciones angulares.\n"
            f"Mediciones válidas: {valid}."
        )

    def export_csv(self):
        if not self.results:
            messagebox.showwarning(
                "Sin resultados",
                "Primero ejecutá el análisis."
            )
            return

        path = filedialog.asksaveasfilename(
            title="Guardar resultados",
            defaultextension=".csv",
            filetypes=[("CSV", "*.csv"), ("TXT", "*.txt")]
        )

        if not path:
            return

        try:
            with open(path, "w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)

                # Formato simple para que Arduino pueda procesarlo.
                writer.writerow(["G99"])

                for angle, distance in self.results:
                    if distance is None:
                        writer.writerow([
                            f"{angle:.1f}",
                            ""
                        ])
                    else:
                        writer.writerow([
                            f"{distance:.4f}"
                        ])
                writer.writerow(["$"])
            self.status_var.set(f"Archivo exportado: {path}")

            messagebox.showinfo(
                "Exportación",
                f"Archivo guardado correctamente:\n\n{path}"
            )

        except Exception as e:
            messagebox.showerror(
                "Error",
                f"No se pudo guardar el archivo:\n\n{e}"
            )


def main():
    root = tk.Tk()
    app = RadialAnalyzer(root)
    root.mainloop()


if __name__ == "__main__":
    main()
