"""
Analizador de Perfil Radial CAD + Control Serial

Dependencias:
    pip install ezdxf numpy pyserial

Características:
- Carga DXF.
- Detecta el centro del CIRCLE de mayor radio.
- Excluye ese círculo de referencia del análisis.
- Mide la INTERSECCIÓN MÁS CERCANA al centro en cada ángulo.
- 0° apunta hacia la derecha.
- Resolución angular de 0.5° a 2.0° en pasos de 0.1°.
- Comunicación serie con selección de COM y baudrate.
- Pestaña Control de movimiento:
    X/Z: G1 = lejos de home, G2 = hacia home
    Home X/Z: G0 X / G0 Z
    Y rotativo: G1 Y / G2 Y, en grados
    Motor: S1 ON / S1 OFF
- Envía perfil mediante:
    G99 valor1,valor2,valor3,...
  sin ángulos, una sola línea.

Ejecutar:
    python analizador_radial.py
"""

import math
import csv
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText

try:
    import ezdxf
except ImportError:
    raise SystemExit("Falta ezdxf. Ejecutá: pip install ezdxf")

try:
    import numpy as np
except ImportError:
    raise SystemExit("Falta numpy. Ejecutá: pip install numpy")

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    raise SystemExit("Falta pyserial. Ejecutá: pip install pyserial")


class RadialAnalyzer:
    def __init__(self, root):
        self.root = root
        self.root.title("Analizador de Perfil Radial CAD + Control")
        self.root.geometry("1050x720")
        self.root.minsize(900, 620)

        self.doc = None
        self.entities = []
        self.reference_circle = None
        self.center = None
        self.results = []

        self.ser = None

        self.build_ui()
        self.refresh_ports()

    # ------------------------------------------------------------
    # INTERFAZ
    # ------------------------------------------------------------
    def build_ui(self):
        notebook = ttk.Notebook(self.root)
        notebook.pack(fill="both", expand=True, padx=8, pady=8)

        self.tab_analysis = ttk.Frame(notebook, padding=10)
        self.tab_control = ttk.Frame(notebook, padding=10)

        notebook.add(self.tab_analysis, text="Análisis CAD")
        notebook.add(self.tab_control, text="Control de movimiento")

        self.build_analysis_tab()
        self.build_control_tab()

    def build_analysis_tab(self):
        tab = self.tab_analysis

        # Archivo
        file_frame = ttk.LabelFrame(tab, text="Archivo CAD", padding=10)
        file_frame.pack(fill="x", pady=(0, 8))

        self.file_var = tk.StringVar(value="Ningún archivo seleccionado")
        ttk.Label(
            file_frame, textvariable=self.file_var
        ).pack(side="left", fill="x", expand=True)

        ttk.Button(
            file_frame, text="Cargar DXF", command=self.load_dxf
        ).pack(side="right")

        # Centro
        center_frame = ttk.LabelFrame(
            tab, text="Centro de análisis", padding=10
        )
        center_frame.pack(fill="x", pady=(0, 8))

        ttk.Label(center_frame, text="X:").grid(
            row=0, column=0, padx=5
        )
        self.x_var = tk.StringVar(value="0")
        ttk.Entry(
            center_frame, textvariable=self.x_var, width=14
        ).grid(row=0, column=1, padx=5)

        ttk.Label(center_frame, text="Y:").grid(
            row=0, column=2, padx=5
        )
        self.y_var = tk.StringVar(value="0")
        ttk.Entry(
            center_frame, textvariable=self.y_var, width=14
        ).grid(row=0, column=3, padx=5)

        ttk.Button(
            center_frame,
            text="Detectar centro del círculo",
            command=self.detect_circle_center
        ).grid(row=0, column=4, padx=15)

        self.circle_info_var = tk.StringVar(
            value="Círculo de referencia: no detectado"
        )
        ttk.Label(
            center_frame,
            textvariable=self.circle_info_var
        ).grid(row=1, column=0, columnspan=5, sticky="w", pady=(8, 0))

        # Configuración
        config = ttk.LabelFrame(tab, text="Configuración", padding=10)
        config.pack(fill="x", pady=(0, 8))

        ttk.Label(
            config, text="Resolución angular:"
        ).grid(row=0, column=0, padx=5)

        values = [f"{x / 10:.1f}°" for x in range(5, 21)]
        self.res_var = tk.StringVar(value="1.0°")
        ttk.Combobox(
            config,
            textvariable=self.res_var,
            values=values,
            state="readonly",
            width=10
        ).grid(row=0, column=1, padx=5)

        ttk.Label(
            config, text="Sentido:"
        ).grid(row=0, column=2, padx=(30, 5))

        self.direction_var = tk.StringVar(value="Antihorario")
        ttk.Radiobutton(
            config, text="Antihorario",
            variable=self.direction_var,
            value="Antihorario"
        ).grid(row=0, column=3, padx=5)

        ttk.Radiobutton(
            config, text="Horario",
            variable=self.direction_var,
            value="Horario"
        ).grid(row=0, column=4, padx=5)

        ttk.Label(
            config,
            text="0° = derecha  |  90° = arriba  |  180° = izquierda  |  270° = abajo"
        ).grid(row=1, column=0, columnspan=5, sticky="w", pady=(8, 0))

        # Acciones
        buttons = ttk.Frame(tab)
        buttons.pack(fill="x", pady=(0, 8))

        ttk.Button(
            buttons, text="ANALIZAR 360°",
            command=self.analyze
        ).pack(side="left", padx=(0, 8))

        ttk.Button(
            buttons, text="ENVIAR G99",
            command=self.send_g99
        ).pack(side="left", padx=8)

        # Tabla
        table_frame = ttk.LabelFrame(tab, text="Resultados", padding=5)
        table_frame.pack(fill="both", expand=True)

        columns = ("angulo", "distancia")
        self.tree = ttk.Treeview(
            table_frame, columns=columns, show="headings"
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
            tab, textvariable=self.status_var,
            relief="sunken", anchor="w"
        ).pack(fill="x", pady=(8, 0))

    def build_control_tab(self):
        tab = self.tab_control

        # Serie
        serial_frame = ttk.LabelFrame(
            tab, text="Comunicación serie", padding=10
        )
        serial_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(serial_frame, text="Puerto:").grid(
            row=0, column=0, padx=5
        )

        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(
            serial_frame,
            textvariable=self.port_var,
            state="readonly",
            width=18
        )
        self.port_combo.grid(row=0, column=1, padx=5)

        ttk.Button(
            serial_frame,
            text="Actualizar",
            command=self.refresh_ports
        ).grid(row=0, column=2, padx=5)

        ttk.Label(serial_frame, text="Baudrate:").grid(
            row=0, column=3, padx=(20, 5)
        )

        baud_values = [
            "9600", "19200", "38400", "57600",
            "115200", "230400"
        ]
        self.baud_var = tk.StringVar(value="115200")
        ttk.Combobox(
            serial_frame,
            textvariable=self.baud_var,
            values=baud_values,
            state="readonly",
            width=12
        ).grid(row=0, column=4, padx=5)

        self.connect_button = ttk.Button(
            serial_frame,
            text="CONECTAR",
            command=self.toggle_serial
        )
        self.connect_button.grid(row=0, column=5, padx=(20, 5))

        self.serial_status_var = tk.StringVar(value="Desconectado")
        ttk.Label(
            serial_frame,
            textvariable=self.serial_status_var
        ).grid(row=0, column=6, padx=10)

        # Control X/Z
        xz_frame = ttk.LabelFrame(
            tab, text="Movimiento lineal X / Z", padding=15
        )
        xz_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(
            xz_frame, text="Distancia:"
        ).grid(row=0, column=0, padx=5)

        self.linear_distance_var = tk.StringVar(value="1.0")
        ttk.Combobox(
            xz_frame,
            textvariable=self.linear_distance_var,
            values=[
                "0.1", "0.5", "1.0", "2.0", "5.0",
                "10.0", "25.0", "50.0", "100.0"
            ],
            state="readonly",
            width=10
        ).grid(row=0, column=1, padx=5)

        ttk.Label(
            xz_frame, text="mm"
        ).grid(row=0, column=2, padx=(0, 20))

        # Panel de flechas
        arrows = ttk.Frame(xz_frame)
        arrows.grid(row=1, column=0, columnspan=3, pady=15)

        ttk.Button(
            arrows, text="↑\nZ +",
            width=8,
            command=lambda: self.move_linear("Z", True)
        ).grid(row=0, column=1, padx=8, pady=4)

        ttk.Button(
            arrows, text="←\nX −",
            width=8,
            command=lambda: self.move_linear("X", False)
        ).grid(row=1, column=0, padx=8, pady=4)

        ttk.Button(
            arrows, text="HOME\nX",
            width=8,
            command=lambda: self.home_axis("X")
        ).grid(row=1, column=1, padx=8, pady=4)

        ttk.Button(
            arrows, text="→\nX +",
            width=8,
            command=lambda: self.move_linear("X", True)
        ).grid(row=1, column=2, padx=8, pady=4)

        ttk.Button(
            arrows, text="↓\nZ −",
            width=8,
            command=lambda: self.move_linear("Z", False)
        ).grid(row=2, column=1, padx=8, pady=4)

        ttk.Button(
            arrows, text="HOME\nZ",
            width=8,
            command=lambda: self.home_axis("Z")
        ).grid(row=2, column=2, padx=8, pady=4)

        ttk.Label(
            xz_frame,
            text="G1 = lejos de HOME     |     G2 = hacia HOME     |     G0 = HOME"
        ).grid(row=2, column=0, columnspan=3, pady=(5, 0))

        # Control Y
        y_frame = ttk.LabelFrame(
            tab, text="Eje Y rotativo", padding=15
        )
        y_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(
            y_frame, text="Giro:"
        ).grid(row=0, column=0, padx=5)

        self.y_degrees_var = tk.StringVar(value="10.0")
        ttk.Combobox(
            y_frame,
            textvariable=self.y_degrees_var,
            values=[
                "0.1", "0.5", "1.0", "2.0", "5.0",
                "10.0", "15.0", "30.0", "45.0", "90.0", "180.0"
            ],
            state="readonly",
            width=10
        ).grid(row=0, column=1, padx=5)

        ttk.Label(
            y_frame, text="°"
        ).grid(row=0, column=2, padx=(0, 20))

        y_buttons = ttk.Frame(y_frame)
        y_buttons.grid(row=1, column=0, columnspan=3, pady=12)

        ttk.Button(
            y_buttons,
            text="↶  ANTIHORARIO",
            width=18,
            command=lambda: self.rotate_y(False)
        ).grid(row=0, column=0, padx=10)

        ttk.Button(
            y_buttons,
            text="HORARIO  ↷",
            width=18,
            command=lambda: self.rotate_y(True)
        ).grid(row=0, column=1, padx=10)

        ttk.Label(
            y_frame,
            text="Y no se desplaza: solamente gira. Se envía G1/G2 Y con grados."
        ).grid(row=2, column=0, columnspan=3)

        # Motor
        motor_frame = ttk.LabelFrame(
            tab, text="Motor auxiliar", padding=15
        )
        motor_frame.pack(fill="x", pady=(0, 10))

        self.motor_on = False
        self.motor_button = tk.Button(
            motor_frame,
            text="MOTOR OFF",
            command=self.toggle_motor,
            width=18,
            height=2
        )
        self.motor_button.pack()

        ttk.Label(
            motor_frame,
            text="Comandos: S1 ON / S1 OFF"
        ).pack(pady=(5, 0))

        # Terminal
        terminal_frame = ttk.LabelFrame(
            tab, text="Terminal serie", padding=5
        )
        terminal_frame.pack(fill="both", expand=True)

        self.terminal = ScrolledText(
            terminal_frame, height=8, state="disabled"
        )
        self.terminal.pack(fill="both", expand=True)

        command_frame = ttk.Frame(terminal_frame)
        command_frame.pack(fill="x", pady=5)

        self.command_var = tk.StringVar()
        entry = ttk.Entry(
            command_frame, textvariable=self.command_var
        )
        entry.pack(side="left", fill="x", expand=True, padx=(0, 5))
        entry.bind("<Return>", lambda event: self.send_manual())

        ttk.Button(
            command_frame,
            text="Enviar",
            command=self.send_manual
        ).pack(side="right")

    # ------------------------------------------------------------
    # DXF
    # ------------------------------------------------------------
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
            self.reference_circle = None
            self.center = None
            self.results = []

            self.file_var.set(path)
            self.status_var.set(
                f"DXF cargado: {len(self.entities)} entidades."
            )

            # Detectar automáticamente el círculo de referencia.
            self.detect_circle_center(show_message=False)

            if self.reference_circle is not None:
                radius = float(self.reference_circle.dxf.radius)
                self.circle_info_var.set(
                    f"Círculo de referencia: radio {radius:.4f} "
                    f"(diámetro {2*radius:.4f}) - excluido del análisis"
                )

            messagebox.showinfo(
                "DXF cargado",
                f"Archivo cargado correctamente.\n\n"
                f"Entidades: {len(self.entities)}\n"
                f"Se seleccionará el CIRCLE de mayor radio como referencia."
            )

        except Exception as e:
            messagebox.showerror(
                "Error",
                f"No se pudo cargar el DXF:\n\n{e}"
            )

    def detect_circle_center(self, show_message=True):
        if not self.entities:
            if show_message:
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
            self.reference_circle = None
            self.center = None
            self.circle_info_var.set(
                "No se encontró CIRCLE. El centro deberá introducirse manualmente."
            )
            if show_message:
                messagebox.showwarning(
                    "Sin círculos",
                    "No se encontró ninguna entidad CIRCLE."
                )
            return

        # El mayor círculo se considera el círculo de referencia.
        c = max(circles, key=lambda e: float(e.dxf.radius))
        self.reference_circle = c

        self.center = (
            float(c.dxf.center.x),
            float(c.dxf.center.y)
        )

        radius = float(c.dxf.radius)

        self.x_var.set(f"{self.center[0]:.6f}")
        self.y_var.set(f"{self.center[1]:.6f}")

        self.circle_info_var.set(
            f"Círculo de referencia: radio {radius:.4f} "
            f"(diámetro {2*radius:.4f}) - excluido del análisis"
        )

        if show_message:
            messagebox.showinfo(
                "Centro detectado",
                f"Centro:\n"
                f"X = {self.center[0]:.6f}\n"
                f"Y = {self.center[1]:.6f}\n\n"
                f"Radio del círculo: {radius:.6f}"
            )

    def get_center(self):
        try:
            return float(self.x_var.get()), float(self.y_var.get())
        except ValueError:
            raise ValueError(
                "Las coordenadas X/Y del centro no son válidas."
            )

    # ------------------------------------------------------------
    # GEOMETRÍA
    # ------------------------------------------------------------
    def entity_to_segments(self, entity):
        """Convierte entidades DXF a segmentos lineales."""

        typ = entity.dxftype()

        if typ == "LINE":
            p1 = entity.dxf.start
            p2 = entity.dxf.end
            return [(
                (float(p1.x), float(p1.y)),
                (float(p2.x), float(p2.y))
            )]

        if typ == "LWPOLYLINE":
            points = list(entity.get_points("xy"))
            if len(points) < 2:
                return []

            pts = [
                (float(p[0]), float(p[1]))
                for p in points
            ]

            result = [
                (pts[i], pts[i + 1])
                for i in range(len(pts) - 1)
            ]

            if entity.closed:
                result.append((pts[-1], pts[0]))

            return result

        if typ == "POLYLINE":
            verts = list(entity.vertices)
            if len(verts) < 2:
                return []

            pts = [
                (
                    float(v.dxf.location.x),
                    float(v.dxf.location.y)
                )
                for v in verts
            ]

            result = [
                (pts[i], pts[i + 1])
                for i in range(len(pts) - 1)
            ]

            if entity.is_closed:
                result.append((pts[-1], pts[0]))

            return result

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

            # Aproximación de 0.1° para mantener buena precisión.
            step_deg = 0.1
            n = max(8, int(math.ceil((a1 - a0) / step_deg)))

            angles = np.linspace(
                math.radians(a0),
                math.radians(a1),
                n + 1
            )

            pts = [
                (
                    cx + radius * math.cos(a),
                    cy + radius * math.sin(a)
                )
                for a in angles
            ]

            return [
                (pts[i], pts[i + 1])
                for i in range(len(pts) - 1)
            ]

        return []

    def build_geometry(self):
        """
        Construye segmentos excluyendo el CIRCLE de referencia.
        Esto evita que el círculo circunscripto sea interpretado como
        parte del perfil de la pieza.
        """
        segments = []

        for entity in self.entities:
            if self.reference_circle is not None:
                if entity is self.reference_circle:
                    continue

            segments.extend(self.entity_to_segments(entity))

        return segments

    @staticmethod
    def ray_segment_intersection(cx, cy, dx, dy, p1, p2):
        """
        P = centro + t*dirección, t >= 0
        Q = p1 + u*(p2-p1), 0 <= u <= 1

        Devuelve t, que en este caso es la distancia radial.
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

    # ------------------------------------------------------------
    # ANÁLISIS
    # ------------------------------------------------------------
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
            resolution = float(
                self.res_var.get().replace("°", "")
            )
        except ValueError:
            messagebox.showerror(
                "Error", "Resolución angular inválida."
            )
            return

        segments = self.build_geometry()

        if not segments:
            messagebox.showerror(
                "Error",
                "No se encontró geometría compatible para analizar."
            )
            return

        count = int(round(360.0 / resolution))
        base_angles = [i * resolution for i in range(count)]

        # 0° siempre es derecha. El sentido sólo cambia el orden de
        # los datos enviados/mostrados.
        if self.direction_var.get() == "Horario":
            angles = [0.0] + [
                360.0 - i * resolution for i in range(1, count)
            ]
        else:
            angles = base_angles

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

            # IMPORTANTE:
            # Ahora se toma la intersección MÁS CERCANA al centro.
            distance = min(distances) if distances else None

            self.results.append((angle_deg, distance))

        for item in self.tree.get_children():
            self.tree.delete(item)

        valid = 0

        for angle, distance in self.results:
            value = "" if distance is None else f"{distance:.4f}"

            if distance is not None:
                valid += 1

            self.tree.insert(
                "", "end",
                values=(f"{angle:.1f}", value)
            )

        self.status_var.set(
            f"Análisis terminado: {len(self.results)} ángulos, "
            f"{valid} mediciones válidas. 0° = derecha."
        )

        messagebox.showinfo(
            "Análisis terminado",
            f"Se analizaron {len(self.results)} posiciones.\n"
            f"Mediciones válidas: {valid}\n\n"
            f"Se tomó la INTERSECCIÓN MÁS CERCANA al centro."
        )

    # ------------------------------------------------------------
    # G99
    # ------------------------------------------------------------
    def make_g99(self):
        if not self.results:
            raise ValueError(
                "Primero ejecutá el análisis."
            )

        missing = [
            (angle, distance)
            for angle, distance in self.results
            if distance is None
        ]

        if missing:
            raise ValueError(
                f"Hay {len(missing)} ángulos sin intersección. "
                "No se enviará G99 hasta resolverlos."
            )

        values = [
            f"{distance:.4f}"
            for _, distance in self.results
        ]

        return "G99 " + ",".join(values)

    def send_g99(self):
        try:
            command = self.make_g99()
        except ValueError as e:
            messagebox.showwarning("G99", str(e))
            return

        if not self.is_serial_connected():
            messagebox.showwarning(
                "Serie",
                "No hay una conexión serie activa."
            )
            return

        self.send_serial(command)

    # ------------------------------------------------------------
    # SERIE
    # ------------------------------------------------------------
    def refresh_ports(self):
        ports = [
            p.device
            for p in serial.tools.list_ports.comports()
        ]

        self.port_combo["values"] = ports

        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
        else:
            self.port_var.set("")

    def is_serial_connected(self):
        return self.ser is not None and self.ser.is_open

    def toggle_serial(self):
        if self.is_serial_connected():
            try:
                self.ser.close()
            except Exception:
                pass

            self.ser = None
            self.connect_button.config(text="CONECTAR")
            self.serial_status_var.set("Desconectado")
            self.log_terminal("[SERIE] Desconectado")
            return

        port = self.port_var.get()

        if not port:
            messagebox.showwarning(
                "Serie",
                "Seleccioná un puerto COM."
            )
            return

        try:
            baud = int(self.baud_var.get())

            self.ser = serial.Serial(
                port=port,
                baudrate=baud,
                timeout=0.05
            )

            self.connect_button.config(text="DESCONECTAR")
            self.serial_status_var.set(
                f"Conectado a {port} @ {baud}"
            )
            self.log_terminal(
                f"[SERIE] Conectado a {port} @ {baud}"
            )

        except Exception as e:
            self.ser = None
            messagebox.showerror(
                "Error de conexión",
                f"No se pudo abrir {port}:\n\n{e}"
            )

    def send_serial(self, command):
        command = command.strip()

        if not command:
            return False

        if not self.is_serial_connected():
            messagebox.showwarning(
                "Serie",
                "No hay una conexión serie activa."
            )
            return False

        try:
            self.ser.write((command + "\n").encode("ascii"))
            self.ser.flush()

            self.log_terminal(f">> {command}")
            return True

        except Exception as e:
            self.log_terminal(f"[ERROR] {e}")
            messagebox.showerror(
                "Error serie",
                f"No se pudo enviar el comando:\n\n{e}"
            )
            return False

    def send_manual(self):
        command = self.command_var.get().strip()

        if self.send_serial(command):
            self.command_var.set("")

    def log_terminal(self, text):
        self.terminal.config(state="normal")
        self.terminal.insert("end", text + "\n")
        self.terminal.see("end")
        self.terminal.config(state="disabled")

    # ------------------------------------------------------------
    # MOVIMIENTO
    # ------------------------------------------------------------
    def get_linear_distance(self):
        try:
            value = float(self.linear_distance_var.get())
            if value <= 0:
                raise ValueError
            return value
        except ValueError:
            raise ValueError(
                "La distancia lineal debe ser mayor que 0."
            )

    def get_y_degrees(self):
        try:
            value = float(self.y_degrees_var.get())
            if value <= 0:
                raise ValueError
            return value
        except ValueError:
            raise ValueError(
                "Los grados de Y deben ser mayores que 0."
            )

    def move_linear(self, axis, away_from_home):
        try:
            distance = self.get_linear_distance()
        except ValueError as e:
            messagebox.showwarning("Movimiento", str(e))
            return

        command_type = "G1" if away_from_home else "G2"
        command = f"{command_type} {axis}{self.format_number(distance)}"

        self.send_serial(command)

    def home_axis(self, axis):
        # Según el protocolo indicado por el usuario:
        # G0 X / G0 Z hacen HOME.
        self.send_serial(f"G0 {axis}")

    def rotate_y(self, clockwise):
        try:
            degrees = self.get_y_degrees()
        except ValueError as e:
            messagebox.showwarning("Movimiento Y", str(e))
            return

        # Convención:
        # G1 = lejos de home
        # G2 = hacia home
        #
        # Para el eje rotativo dejamos:
        # G1 Y = horario
        # G2 Y = antihorario
        command_type = "G1" if clockwise else "G2"
        command = f"{command_type} Y{self.format_number(degrees)}"

        self.send_serial(command)

    @staticmethod
    def format_number(value):
        if float(value).is_integer():
            return str(int(value))
        return f"{float(value):.4f}".rstrip("0").rstrip(".")

    def toggle_motor(self):
        new_state = not self.motor_on
        command = "S1 ON" if new_state else "S1 OFF"

        if self.send_serial(command):
            self.motor_on = new_state

            if self.motor_on:
                self.motor_button.config(
                    text="MOTOR ON",
                    relief="sunken"
                )
            else:
                self.motor_button.config(
                    text="MOTOR OFF",
                    relief="raised"
                )


def main():
    root = tk.Tk()
    app = RadialAnalyzer(root)

    def on_close():
        if app.ser is not None:
            try:
                app.ser.close()
            except Exception:
                pass
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
