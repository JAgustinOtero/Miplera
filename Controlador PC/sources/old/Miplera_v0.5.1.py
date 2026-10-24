"""
Analizador de Perfil Radial CAD + Control Serial V3 (CustomTkinter)
Jerarquía de paleta personalizada dinámicamente adaptable para Modo Oscuro y Modo Claro.
"""

import math
import customtkinter as ctk
from tkinter import filedialog, messagebox, ttk

try:
    import ezdxf
except ImportError:
    raise SystemExit("Falta ezdxf. Instalá: pip install ezdxf")

try:
    import numpy as np
except ImportError:
    raise SystemExit("Falta numpy. Instalá: pip install numpy")

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    raise SystemExit("Falta pyserial. Instalá: pip install pyserial")

# Configuración global de apariencia inicial
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

# ============================================================
# PALETA DE COLORES DINÁMICA (Modo Claro, Modo Oscuro)
# ============================================================
COLOR_BG = ("#EAEAEA", "#202020")          # 1. Fondo principal de la ventana
COLOR_CONTAINER = ("#F5F5F5", "#303030")   # 2. Ventana general / Tabview
COLOR_BOX_CARD = ("#FFFFFF", "#404040")    # 3. Boxes / Cards de cada sección/menú

# Botones Acción Principal
COLOR_BTN_ACTION = ("#1C5180", "#104040")   # Claro: Azul elegante | Oscuro: Verde azulado profundo
COLOR_BTN_HOVER = ("#2B6CB0", "#1C5180")    # Hover adaptado para ambos modos

# Cajas Dropdowns / OptionMenu
COLOR_FG_DROPDOWN = ("#E0E0E0", "#303030")
COLOR_TEXT_DROPDOWN = ("#101010", "#FFFFFF")  # Texto dentro de los dropdowns

# Botones Secundarios / Especiales
COLOR_BTN_HOME = ("#D97706", "#B45309")     # Home (Naranja)
COLOR_BTN_HOME_HOVER = ("#B45309", "#7C2D12")

COLOR_BTN_START = ("#16A34A", "#15803D")    # Inicio / G99 (Verde activo)
COLOR_BTN_START_HOVER = ("#15803D", "#166534")
COLOR_BTN_DISABLED = "#404040"              # Gris inactivo para G99 e Inicio

# Motor Auxiliar (OFF State)
COLOR_BTN_AUX_OFF = ("#6B7280", "#4B5563")  # Gris neutro adaptable
COLOR_BTN_AUX_OFF_HOVER = ("#4B5563", "#374151")


class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        width = self.winfo_screenwidth()
        height = self.winfo_screenheight()

        self.title("MipSlicer v1.0.0")
        self.geometry(f"{width}x{height-40}+0+0")
        self.minsize(800, 600)

        # Fondo dinámico principal
        self.configure(fg_color=COLOR_BG)

        # Variables de estado
        self.doc = None
        self.entities = []
        self.reference_circle = None
        self.center = None
        self.results = []
        self.ser = None
        self.serial_reader_running = False
        self.motor_on = False
        self.g99_sent = False
        self.position_sent = False  # NUEVO: Estado para rastrear si se envió la posición
        self.help_window = None

        self.build_ui()
        self.refresh_ports()
        self.update_treeview_theme("Dark")
        self.after(30, self.read_serial)
        self.protocol("WM_DELETE_WINDOW", self.close)

    def create_card_frame(self, parent, **kwargs):
        """Crea las cajas/tarjetas internas adaptables según el modo."""
        border_col = kwargs.pop("border_color", ("#D0D7DE", "#303030"))
        bg_col = kwargs.pop("fg_color", COLOR_BOX_CARD)
        radius = kwargs.pop("corner_radius", 12)

        frame = ctk.CTkFrame(
            parent,
            fg_color=bg_col,
            border_color=border_col,
            border_width=1,
            corner_radius=radius,
            **kwargs
        )
        return frame

    def set_btn_state(self, btn, enabled: bool):
        """Helper para activar/desactivar botones ajustando su color adecuadamente."""
        if enabled:
            btn.configure(
                state="normal",
                fg_color=COLOR_BTN_START,
                hover_color=COLOR_BTN_START_HOVER
            )
        else:
            btn.configure(
                state="disabled",
                fg_color=COLOR_BTN_DISABLED,
                hover_color=COLOR_BTN_DISABLED
            )

    def update_start_button_state(self):
        """NUEVO: Evalúa si se cumplen ambas condiciones para habilitar el botón INICIO."""
        if self.g99_sent and self.position_sent:
            self.set_btn_state(self.start_button, True)
        else:
            self.set_btn_state(self.start_button, False)

    # ============================================================
    # INTERFAZ GRÁFICA (UI)
    # ============================================================
    def build_ui(self):
        # Header principal
        header = ctk.CTkFrame(self, fg_color="transparent")
        header.pack(fill="x", padx=15, pady=(10, 5))

        title_label = ctk.CTkLabel(
            header, text="MipSlicer v1.0.0", font=("Segoe UI", 24, "bold")
        )
        title_label.pack(side="left")

        # Contenedor derecho en el header para los controles
        header_right = ctk.CTkFrame(header, fg_color="transparent")
        header_right.pack(side="right")

        # Switch de Modo Oscuro
        self.theme_switch = ctk.CTkSwitch(
            header_right,
            text="Modo Oscuro",
            font=("Segoe UI", 13),
            command=self.toggle_theme,
            onvalue="Dark",
            offvalue="Light",
            progress_color=COLOR_BTN_ACTION,
            button_color=COLOR_BTN_HOVER,
            button_hover_color=COLOR_BTN_ACTION
        )
        self.theme_switch.pack(side="left", padx=(0, 10))
        self.theme_switch.select()

        # Botón de Ayuda
        btn_help = ctk.CTkButton(
            header_right,
            text=" help ",
            width=70,
            height=28,
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.open_help_window
        )
        btn_help.pack(side="left")

        # Tabs (Pestañas - Ventana General)
        self.tabview = ctk.CTkTabview(
            self,
            corner_radius=12,
            fg_color=COLOR_CONTAINER,
            segmented_button_selected_color=COLOR_BTN_ACTION,
            segmented_button_selected_hover_color=COLOR_BTN_HOVER
        )
        self.tabview._segmented_button.configure(font=("Segoe UI", 14, "bold"))
        self.tabview.pack(fill="both", expand=True, padx=15, pady=10)

        self.tab_analysis_name = "  Análisis CAD  "
        self.tab_control_name = "  Control y Terminal  "

        self.analysis_tab = self.tabview.add(self.tab_analysis_name)
        self.control_tab = self.tabview.add(self.tab_control_name)

        self.build_analysis_tab()
        self.build_control_tab()

    def open_help_window(self):
        """Abre o enfoca la ventana emergente con la guía de ayuda."""
        if self.help_window is not None and self.help_window.winfo_exists():
            self.help_window.focus()
            return

        self.help_window = ctk.CTkToplevel(self)
        self.help_window.title("Guía de Uso - MipSlicer")
        self.help_window.geometry("650x550")
        self.help_window.minsize(500, 400)
        self.help_window.grab_set()

        lbl_title = ctk.CTkLabel(
            self.help_window,
            text="📖 Guía de Uso Básica",
            font=("Segoe UI", 18, "bold")
        )
        lbl_title.pack(padx=20, pady=(15, 10), anchor="w")

        help_textbox = ctk.CTkTextbox(
            self.help_window,
            font=("Segoe UI", 12),
            corner_radius=8,
            wrap="word"
        )
        help_textbox.pack(fill="both", expand=True, padx=15, pady=(0, 15))

        help_text = (
            "------------------------ MODO DE USO: --------------------------\n"
            "--------------------------------------------------\n"
            "1. Conexión Serie: Selecciona el Puerto COM de tu dispositivo (ej. Arduino). (Usa el botón 'Actualizar' si no aparece). Selecciona el Baudrate (por defecto 115200). Presiona CONECTAR.\n"
            "2. Cargar DXF: Haz clic en el botón 'Cargar DXF' y selecciona tu archivo .dxf.\n"
            "3. Centro y Referencia: Haz clic en 'Detectar centro' para encontrar automáticamente la entidad CIRCLE más grande dentro del archivo. Si no hay un círculo de referencia, puedes ingresar las coordenadas X e Y manualmente.\n"
            "4. Configurar el Análisis: Selecciona la Resolución angular deseada (p. ej., 1.0°). Elige el sentido de barrido (Antihorario u Horario).\n"
            "5. Ejecutar: Haz clic en 'ANALIZAR 360°'. La tabla se llenará con los pares de Ángulo (°) y Distancia (mm).\n"
            "6. Enviar DATOS: Una vez completado el análisis, se habilitará el botón 'ENVIAR DATOS'. Al presionarlo, el vector de distancias se enviará por la conexión serie activa y te redirigirá a la pestaña de control.\n\n"
            "Pestaña: Control y Terminal\n"
            "7. Enviar Posición: Utiliza los botones en el panel de control para bajar el eje Z hasta que toque con la varilla de madera. Selecciona el diámetro en mm y presiona 'Enviar posicion' para transmitir la posicion y magnitud de la varilla.\n"
            "8. Inicio de Trabajo: El botón 'INICIO' se habilitará automáticamente una vez enviado el comando 'ENVIAR DATOS' (G99) Y presionado el botón 'Enviar posicion'.\n"
            "------------------------ FUNCIONES ADICIONALES ------------------------\n"
            "1. Panel de Control Manual: Configura las magnitudes de paso para los ejes lineales (X/Z) y rotacional (Y). Utiliza la cruceta de botones para mover los ejes, ejecutar giros o volver a la posición de origen (HOME).\n"
            "2. Motor Auxiliar: Activa o desactiva la sierra (S1 ON / S1 OFF).\n"
            "3. Terminal Serie: Monitoriza las respuestas recibidas en tiempo real. Envía comandos en G-Code de forma manual mediante el cuadro de texto inferior."
        )

        help_textbox.insert("1.0", help_text)
        help_textbox.configure(state="disabled")

    def build_analysis_tab(self):
        tab = self.analysis_tab

        # 1. Cargar DXF
        file_frame = self.create_card_frame(tab)
        file_frame.pack(fill="x", padx=5, pady=(5, 5))

        self.file_var = ctk.StringVar(value="Ningún DXF seleccionado")
        ttk_label = ctk.CTkLabel(
            file_frame,
            textvariable=self.file_var,
            font=("Segoe UI", 13),
            anchor="w",
        )
        ttk_label.pack(side="left", fill="x", expand=True, padx=15, pady=10)

        ctk.CTkButton(
            file_frame,
            text="Cargar DXF",
            corner_radius=8,
            font=("Segoe UI", 13, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.load_dxf,
        ).pack(side="right", padx=10, pady=10)

        # 2. Configuración (Centro + Parámetros)
        top_config = ctk.CTkFrame(tab, fg_color="transparent")
        top_config.pack(fill="x", padx=0, pady=5)
        top_config.columnconfigure(0, weight=1)
        top_config.columnconfigure(1, weight=1)

        # Centro (Izquierda)
        center_box = self.create_card_frame(top_config)
        center_box.grid(row=0, column=0, sticky="nsew", padx=(5, 5), pady=5)

        ctk.CTkLabel(
            center_box,
            text="Centro y Referencia",
            font=("Segoe UI", 14, "bold"),
        ).pack(anchor="w", padx=12, pady=(8, 4))

        c_inner = ctk.CTkFrame(center_box, fg_color="transparent")
        c_inner.pack(pady=5)

        ctk.CTkLabel(c_inner, text="X:", font=("Segoe UI", 13)).grid(row=0, column=0, padx=2)
        self.x_var = ctk.StringVar(value="0")
        ctk.CTkEntry(
            c_inner, textvariable=self.x_var, width=80, corner_radius=6, font=("Segoe UI", 13)
        ).grid(row=0, column=1, padx=4)

        ctk.CTkLabel(c_inner, text="Y:", font=("Segoe UI", 13)).grid(row=0, column=2, padx=2)
        self.y_var = ctk.StringVar(value="0")
        ctk.CTkEntry(
            c_inner, textvariable=self.y_var, width=80, corner_radius=6, font=("Segoe UI", 13)
        ).grid(row=0, column=3, padx=4)

        ctk.CTkButton(
            c_inner,
            text="Detectar centro",
            corner_radius=6,
            width=110,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.detect_circle_center,
        ).grid(row=0, column=4, padx=6)

        self.circle_info_var = ctk.StringVar(
            value="Círculo de referencia: no detectado"
        )
        ctk.CTkLabel(
            center_box,
            textvariable=self.circle_info_var,
            font=("Segoe UI", 11),
            text_color=("gray40", "gray75"),
        ).pack(anchor="w", padx=12, pady=(0, 8))

        # Parámetros (Derecha)
        config_box = self.create_card_frame(top_config)
        config_box.grid(row=0, column=1, sticky="nsew", padx=(5, 5), pady=5)

        ctk.CTkLabel(
            config_box,
            text="Configuración del Análisis",
            font=("Segoe UI", 14, "bold"),
        ).pack(anchor="w", padx=12, pady=(8, 4))

        cfg_inner = ctk.CTkFrame(config_box, fg_color="transparent")
        cfg_inner.pack(pady=5)

        ctk.CTkLabel(cfg_inner, text="Resolución:", font=("Segoe UI", 13)).grid(
            row=0, column=0, padx=2
        )
        self.res_var = ctk.StringVar(value="1.0°")
        ctk.CTkOptionMenu(
            cfg_inner,
            variable=self.res_var,
            values=[f"{i/10:.1f}°" for i in range(5, 21)],
            width=90,
            corner_radius=6,
            font=("Segoe UI", 13),
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_color=COLOR_BTN_ACTION,
            button_hover_color=COLOR_BTN_HOVER
        ).grid(row=0, column=1, padx=4)

        self.direction_var = ctk.StringVar(value="Antihorario")
        ctk.CTkRadioButton(
            cfg_inner,
            text="Antihorario",
            variable=self.direction_var,
            value="Antihorario",
            font=("Segoe UI", 13),
            fg_color=COLOR_BTN_ACTION
        ).grid(row=0, column=2, padx=6)
        ctk.CTkRadioButton(
            cfg_inner,
            text="Horario",
            variable=self.direction_var,
            value="Horario",
            font=("Segoe UI", 13),
            fg_color=COLOR_BTN_ACTION
        ).grid(row=0, column=3, padx=6)

        ctk.CTkLabel(
            config_box,
            text="0° = derecha • 90° = arriba • 180° = izquierda • 270° = abajo",
            font=("Segoe UI", 12),
            text_color=("gray40", "gray75"),
        ).pack(anchor="w", padx=12, pady=(0, 8))

        # 3. Botones de Acción principales
        action_frame = ctk.CTkFrame(tab, fg_color="transparent")
        action_frame.pack(fill="x", padx=5, pady=5)

        action_frame.columnconfigure(0, weight=8)
        action_frame.columnconfigure(1, weight=2)

        ctk.CTkButton(
            action_frame,
            text="ANALIZAR 360°",
            corner_radius=8,
            height=38,
            font=("Segoe UI", 13, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.analyze,
        ).grid(row=0, column=0, sticky="ew", padx=(0, 5))

        self.btn_send_g99 = ctk.CTkButton(
            action_frame,
            text="ENVIAR DATOS",
            corner_radius=8,
            height=38,
            font=("Segoe UI", 13, "bold"),
            command=self.send_g99,
        )
        self.btn_send_g99.grid(row=0, column=1, sticky="ew", padx=(5, 0))
        self.set_btn_state(self.btn_send_g99, False)

        # Perfil Radial (Tabla Treeview dentro de caja)
        table_frame = self.create_card_frame(tab)
        table_frame.pack(fill="both", expand=True, padx=5, pady=5)

        self.style = ttk.Style()
        self.style.theme_use("clam")

        self.tree = ttk.Treeview(
            table_frame, columns=("angulo", "distancia"), show="headings"
        )
        self.tree.heading("angulo", text="Ángulo (°)")
        self.tree.heading("distancia", text="Distancia (mm)")
        self.tree.column("angulo", width=180, anchor="center")
        self.tree.column("distancia", width=220, anchor="center")

        sb = ttk.Scrollbar(
            table_frame, orient="vertical", command=self.tree.yview
        )
        self.tree.configure(yscrollcommand=sb.set)

        self.tree.pack(side="left", fill="both", expand=True, padx=8, pady=8)
        sb.pack(side="right", fill="y", pady=8, padx=(0, 8))

        self.status_var = ctk.StringVar(value="Listo.")
        ctk.CTkLabel(
            tab, textvariable=self.status_var, anchor="w", font=("Segoe UI", 11)
        ).pack(fill="x", padx=10, pady=(2, 5))

    def build_control_tab(self):
        tab = self.control_tab

        # Comunicaciones Serie
        serial_frame = self.create_card_frame(tab)
        serial_frame.pack(fill="x", padx=5, pady=5)

        s_inner = ctk.CTkFrame(serial_frame, fg_color="transparent")
        s_inner.pack(padx=10, pady=8, fill="x")

        ctk.CTkLabel(s_inner, text="Puerto:", font=("Segoe UI", 13)).pack(side="left", padx=4)
        self.port_var = ctk.StringVar()
        self.port_combo = ctk.CTkOptionMenu(
            s_inner,
            variable=self.port_var,
            values=[],
            width=130,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        )
        self.port_combo.pack(side="left", padx=4)

        ctk.CTkButton(
            s_inner,
            text="Actualizar",
            width=90,
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.refresh_ports,
        ).pack(side="left", padx=4)

        ctk.CTkLabel(s_inner, text="Baudrate:", font=("Segoe UI", 13)).pack(side="left", padx=(15, 4))
        self.baud_var = ctk.StringVar(value="115200")
        ctk.CTkOptionMenu(
            s_inner,
            variable=self.baud_var,
            values=["9600", "19200", "38400", "57600", "115200", "230400"],
            width=110,
            corner_radius=6,
            font=("Segoe UI", 13),
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_color=COLOR_BTN_ACTION,
            button_hover_color=COLOR_BTN_HOVER
        ).pack(side="left", padx=4)

        self.connect_button = ctk.CTkButton(
            s_inner,
            text="CONECTAR",
            corner_radius=6,
            font=("Segoe UI", 13, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.toggle_serial,
        )
        self.connect_button.pack(side="left", padx=15)

        self.serial_status_var = ctk.StringVar(value="● Desconectado")
        self.lbl_serial_status = ctk.CTkLabel(
            s_inner,
            textvariable=self.serial_status_var,
            font=("Segoe UI", 13),
            text_color="#EF4444"
        )
        self.lbl_serial_status.pack(side="left", padx=10)

        # Layout Split
        main_split = ctk.CTkFrame(tab, fg_color="transparent")
        main_split.pack(fill="both", expand=True, padx=0, pady=5)

        main_split.rowconfigure(0, weight=1)
        main_split.columnconfigure(0, weight=4)
        main_split.columnconfigure(1, weight=6)

        # Panel Izquierdo: Control Integrado
        left_panel = self.create_card_frame(main_split)
        left_panel.grid(row=0, column=0, sticky="nsew", padx=(5, 5), pady=5)

        self.build_unified_controls(left_panel)

        # Panel Derecho: Terminal Serie
        right_panel = self.create_card_frame(main_split)
        right_panel.grid(row=0, column=1, sticky="nsew", padx=(5, 5), pady=5)

        ctk.CTkLabel(
            right_panel, text="Terminal Serie", font=("Segoe UI", 14, "bold")
        ).pack(anchor="w", padx=12, pady=(8, 4))

        self.terminal = ctk.CTkTextbox(
            right_panel, font=("Consolas", 12), corner_radius=8
        )
        self.terminal.pack(fill="both", expand=True, padx=10, pady=5)

        cmd_frame = ctk.CTkFrame(right_panel, fg_color="transparent")
        cmd_frame.pack(fill="x", padx=10, pady=8)

        self.command_var = ctk.StringVar()
        self.command_entry = ctk.CTkEntry(
            cmd_frame,
            textvariable=self.command_var,
            placeholder_text="Enviar comando...",
            corner_radius=6,
            font=("Segoe UI", 13),
        )
        self.command_entry.pack(
            side="left", fill="x", expand=True, padx=(0, 6)
        )
        self.command_entry.bind("<Return>", lambda e: self.send_manual())

        ctk.CTkButton(
            cmd_frame,
            text="ENVIAR",
            width=80,
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.send_manual,
        ).pack(side="right")

        ctk.CTkButton(
            cmd_frame,
            text="Limpiar",
            width=80,
            fg_color=("gray60", "gray40"),
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            command=self.clear_terminal,
        ).pack(side="right", padx=6)

        self.log_terminal(
            "Terminal lista. Los mensajes del Arduino se mostrarán aquí."
        )

    def build_unified_controls(self, parent):
        container = ctk.CTkFrame(parent, fg_color="transparent")
        container.pack(expand=True, pady=10)

        # ----------------------------------------------------
        # CONTROL: Diámetro varilla (mm) + Botón Enviar Posición
        # ----------------------------------------------------
        rod_frame = ctk.CTkFrame(container, fg_color="transparent")
        rod_frame.pack(pady=(0, 10))

        ctk.CTkLabel(
            rod_frame, text="Diametro varilla (mm):", font=("Segoe UI", 13)
        ).pack(side="left", padx=4)

        self.rod_diameter_var = ctk.StringVar(value="10")
        ctk.CTkOptionMenu(
            rod_frame,
            variable=self.rod_diameter_var,
            values=[str(i) for i in range(10, 31)],
            width=70,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        ).pack(side="left", padx=4)

        ctk.CTkButton(
            rod_frame,
            text="Enviar posicion",
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.send_rod_diameter,
        ).pack(side="left", padx=6)

        # Selector de Pasos
        selectors = ctk.CTkFrame(container, fg_color="transparent")
        selectors.pack(pady=(0, 10))

        ctk.CTkLabel(selectors, text="Movimiento X/Z:", font=("Segoe UI", 13)).pack(side="left", padx=2)
        self.linear_distance_var = ctk.StringVar(value="1.0")
        ctk.CTkOptionMenu(
            selectors,
            variable=self.linear_distance_var,
            values=["0.1", "0.5", "1.0", "2.0", "5.0", "10.0", "25.0", "50.0", "100.0"],
            width=80,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        ).pack(side="left", padx=2)
        ctk.CTkLabel(selectors, text="mm", font=("Segoe UI", 13)).pack(side="left", padx=(0, 10))

        ctk.CTkLabel(selectors, text="Rotacion Y:", font=("Segoe UI", 13)).pack(side="left", padx=2)
        self.y_degrees_var = ctk.StringVar(value="10.0°")
        ctk.CTkOptionMenu(
            selectors,
            variable=self.y_degrees_var,
            values=["0.1°", "0.5°", "1.0°", "2.0°", "5.0°", "10.0°", "15.0°", "30.0°", "45.0°", "90.0°", "180.0°"],
            width=80,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        ).pack(side="left", padx=2)

        # Botones de movimiento
        grid_frame = ctk.CTkFrame(container, fg_color="transparent")
        grid_frame.pack(pady=4)

        btn_kwargs = {
            "width": 75,
            "height": 60,
            "corner_radius": 10,
            "font": ("Segoe UI", 13, "bold"),
            "fg_color": COLOR_BTN_ACTION,
            "hover_color": COLOR_BTN_HOVER
        }

        # Giro Antihorario
        ctk.CTkButton(
            grid_frame,
            text="↶\nANTI",
            command=lambda: self.rotate_y(False),
            **btn_kwargs,
        ).grid(row=0, column=0, padx=4, pady=4)

        # Z -
        ctk.CTkButton(
            grid_frame,
            text="↑\nZ −",
            command=lambda: self.move_linear("Z", False),
            **btn_kwargs,
        ).grid(row=0, column=1, padx=4, pady=4)

        # X +
        ctk.CTkButton(
            grid_frame,
            text="←\nX +",
            command=lambda: self.move_linear("X", True),
            **btn_kwargs,
        ).grid(row=1, column=0, padx=4, pady=4)

        # HOME
        ctk.CTkButton(
            grid_frame,
            text="⌂\nHOME",
            fg_color=COLOR_BTN_HOME,
            hover_color=COLOR_BTN_HOME_HOVER,
            command=self.home_general,
            width=btn_kwargs["width"],
            height=btn_kwargs["height"],
            corner_radius=btn_kwargs["corner_radius"],
            font=btn_kwargs["font"]
        ).grid(row=1, column=1, padx=4, pady=4)

        # X -
        ctk.CTkButton(
            grid_frame,
            text="→\nX −",
            command=lambda: self.move_linear("X", False),
            **btn_kwargs,
        ).grid(row=1, column=2, padx=4, pady=4)

        # Z +
        ctk.CTkButton(
            grid_frame,
            text="↓\nZ +",
            command=lambda: self.move_linear("Z", True),
            **btn_kwargs,
        ).grid(row=2, column=1, padx=4, pady=4)

        # Giro Horario
        ctk.CTkButton(
            grid_frame,
            text="↷\nHORA",
            command=lambda: self.rotate_y(True),
            **btn_kwargs,
        ).grid(row=0, column=2, padx=4, pady=4)

        # Homes individuales
        homes = ctk.CTkFrame(container, fg_color="transparent")
        homes.pack(pady=6)

        ctk.CTkButton(
            homes,
            text="HOME X",
            corner_radius=6,
            width=100,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=lambda: self.home_axis("X"),
        ).pack(side="left", padx=4)

        ctk.CTkButton(
            homes,
            text="HOME Z",
            corner_radius=6,
            width=100,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=lambda: self.home_axis("Z"),
        ).pack(side="left", padx=4)

        # Motor Auxiliar
        self.motor_button = ctk.CTkButton(
            container,
            text="MOTOR SIERRA: OFF",
            height=40,
            corner_radius=8,
            fg_color=COLOR_BTN_AUX_OFF,
            hover_color=COLOR_BTN_AUX_OFF_HOVER,
            font=("Segoe UI", 12, "bold"),
            command=self.toggle_motor,
        )
        self.motor_button.pack(pady=(4, 2), fill="x")

        # Botón INICIO
        self.start_button = ctk.CTkButton(
            container,
            text="INICIO",
            height=40,
            corner_radius=8,
            font=("Segoe UI", 12, "bold"),
            command=self.send_inicio,
        )
        self.start_button.pack(pady=(2, 4), fill="x")
        self.set_btn_state(self.start_button, False)

    def toggle_theme(self):
        mode = self.theme_switch.get()
        ctk.set_appearance_mode(mode)
        self.update_treeview_theme(mode)

    def update_treeview_theme(self, mode):
        """Aplica los colores de la tabla respetando el tema activo."""
        if mode == "Dark":
            bg = COLOR_BOX_CARD[1]
            fg = "#FFFFFF"
            heading_bg = COLOR_CONTAINER[1]
            heading_fg = "#FFFFFF"
        else:
            bg = COLOR_BOX_CARD[0]
            fg = "#101010"
            heading_bg = COLOR_CONTAINER[0]
            heading_fg = "#101010"

        self.style.configure(
            "Treeview",
            background=bg,
            fieldbackground=bg,
            foreground=fg,
            rowheight=30,
            font=("Segoe UI", 13),
        )
        self.style.configure(
            "Treeview.Heading",
            background=heading_bg,
            foreground=heading_fg,
            font=("Segoe UI", 13, "bold"),
        )

    # ============================================================
    # LÓGICA CAD & ANÁLISIS
    # ============================================================
    def load_dxf(self):
        path = filedialog.askopenfilename(
            title="Seleccionar DXF",
            filetypes=[("DXF", "*.dxf"), ("Todos los archivos", "*.*")],
        )
        if not path:
            return

        try:
            self.doc = ezdxf.readfile(path)
            self.entities = list(self.doc.modelspace())
            self.reference_circle = None
            self.results = []
            
            self.set_btn_state(self.btn_send_g99, False)
            self.g99_sent = False
            self.position_sent = False  # NUEVO: Reinicia la bandera al cargar nuevo archivo
            self.update_start_button_state()  # Deshabilita el botón INICIO

            self.file_var.set(path)
            self.detect_circle_center(show_message=False)
            self.status_var.set(f"DXF cargado: {len(self.entities)} entidades.")
            messagebox.showinfo(
                "DXF cargado",
                f"Archivo cargado correctamente.\n\nEntidades encontradas: {len(self.entities)}",
            )
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo cargar el DXF:\n\n{e}")

    def detect_circle_center(self, show_message=True):
        circles = [e for e in self.entities if e.dxftype() == "CIRCLE"]
        if not circles:
            self.reference_circle = None
            self.center = None
            self.circle_info_var.set(
                "No se encontró CIRCLE. Introducí el centro manualmente."
            )
            if show_message:
                messagebox.showwarning(
                    "Sin círculo", "No se encontró ninguna entidad CIRCLE."
                )
            return

        c = max(circles, key=lambda e: float(e.dxf.radius))
        self.reference_circle = c
        self.center = (float(c.dxf.center.x), float(c.dxf.center.y))
        radius = float(c.dxf.radius)

        self.x_var.set(f"{self.center[0]:.6f}")
        self.y_var.set(f"{self.center[1]:.6f}")
        self.circle_info_var.set(
            f"Referencia: radio {radius:.4f}  |  diámetro {2 * radius:.4f}  |  excluido del perfil"
        )

        if show_message:
            messagebox.showinfo(
                "Centro detectado",
                f"Centro:\nX = {self.center[0]:.6f}\nY = {self.center[1]:.6f}\n\nRadio = {radius:.6f}",
            )

    def get_center(self):
        try:
            return (float(self.x_var.get()), float(self.y_var.get()))
        except ValueError:
            raise ValueError("Centro X/Y inválido.")

    def entity_to_segments(self, entity):
        typ = entity.dxftype()

        if typ == "LINE":
            p1 = entity.dxf.start
            p2 = entity.dxf.end
            return [((float(p1.x), float(p1.y)), (float(p2.x), float(p2.y)))]

        if typ == "LWPOLYLINE":
            points = list(entity.get_points("xy"))
            if len(points) < 2:
                return []
            pts = [(float(p[0]), float(p[1])) for p in points]
            result = [(pts[i], pts[i + 1]) for i in range(len(pts) - 1)]
            if entity.closed:
                result.append((pts[-1], pts[0]))
            return result

        if typ == "POLYLINE":
            verts = list(entity.vertices)
            if len(verts) < 2:
                return []
            pts = [
                (float(v.dxf.location.x), float(v.dxf.location.y))
                for v in verts
            ]
            result = [(pts[i], pts[i + 1]) for i in range(len(pts) - 1)]
            if entity.is_closed:
                result.append((pts[-1], pts[0]))
            return result

        if typ in ("ARC", "CIRCLE"):
            center = entity.dxf.center
            cx, cy = float(center.x), float(center.y)
            radius = float(entity.dxf.radius)

            if typ == "CIRCLE":
                a0, a1 = 0.0, 360.0
            else:
                a0, a1 = float(entity.dxf.start_angle), float(
                    entity.dxf.end_angle
                )
                if a1 <= a0:
                    a1 += 360.0

            n = max(8, int(math.ceil((a1 - a0) / 0.1)))
            angles = np.linspace(math.radians(a0), math.radians(a1), n + 1)
            pts = [
                (cx + radius * math.cos(a), cy + radius * math.sin(a))
                for a in angles
            ]
            return [(pts[i], pts[i + 1]) for i in range(len(pts) - 1)]

        return []

    def build_geometry(self):
        segments = []
        for entity in self.entities:
            if (
                self.reference_circle is not None
                and entity is self.reference_circle
            ):
                continue
            segments.extend(self.entity_to_segments(entity))
        return segments

    @staticmethod
    def ray_segment_intersection(cx, cy, dx, dy, p1, p2):
        x1, y1 = p1
        x2, y2 = p2
        sx, sy = x2 - x1, y2 - y1
        cross = dx * sy - dy * sx

        if abs(cross) < 1e-12:
            return None

        qpx, qpy = x1 - cx, y1 - cy
        t = (qpx * sy - qpy * sx) / cross
        u = (qpx * dy - qpy * dx) / cross

        if t >= 0 and 0 <= u <= 1:
            return t
        return None

    def analyze(self):
        if not self.entities:
            messagebox.showwarning("Análisis", "Primero cargá un DXF.")
            return

        try:
            cx, cy = self.get_center()
            resolution = float(self.res_var.get().replace("°", ""))
        except ValueError as e:
            messagebox.showerror("Error", str(e))
            return

        segments = self.build_geometry()
        if not segments:
            messagebox.showerror(
                "Error", "No se encontró geometría analizable."
            )
            return

        count = int(round(360 / resolution))
        base = [i * resolution for i in range(count)]

        if self.direction_var.get() == "Horario":
            angles = [0.0] + [360.0 - i * resolution for i in range(1, count)]
        else:
            angles = base

        self.results = []
        for angle in angles:
            a = math.radians(angle)
            dx, dy = math.cos(a), math.sin(a)
            distances = []
            for p1, p2 in segments:
                d = self.ray_segment_intersection(cx, cy, dx, dy, p1, p2)
                if d is not None:
                    distances.append(d)
            distance = min(distances) if distances else None
            self.results.append((angle, distance))

        for item in self.tree.get_children():
            self.tree.delete(item)

        valid = 0
        for angle, distance in self.results:
            value = f"{distance:.4f}" if distance is not None else ""
            if distance is not None:
                valid += 1
            self.tree.insert("", "end", values=(f"{angle:.1f}", value))

        self.status_var.set(
            f"Análisis: {len(self.results)} posiciones | {valid} válidas | Intersección más cercana"
        )
        self.set_btn_state(self.btn_send_g99, True)

    def make_g99(self):
        if not self.results:
            raise ValueError("Primero ejecutá el análisis.")
        if any(d is None for _, d in self.results):
            raise ValueError("Hay ángulos sin medición válida.")

        values = [f"{d:.4f}" for _, d in self.results]
        return "G99 " + " ".join(values) + "$"

    def send_g99(self):
        try:
            command = self.make_g99()
        except ValueError as e:
            messagebox.showwarning("G99", str(e))
            return

        if self.send_serial(command):
            self.tabview.set(self.tab_control_name)

    # ============================================================
    # MANEJO SERIE & COMANDOS
    # ============================================================
    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo.configure(values=ports)
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def is_serial_connected(self):
        return self.ser is not None and self.ser.is_open

    def toggle_serial(self):
        if self.is_serial_connected():
            self.serial_reader_running = False
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
            self.connect_button.configure(
                text="CONECTAR", fg_color=COLOR_BTN_ACTION, hover_color=COLOR_BTN_HOVER
            )
            self.serial_status_var.set("● Desconectado")
            self.lbl_serial_status.configure(text_color="#EF4444")
            self.set_btn_state(self.start_button, False)
            self.log_terminal("[SERIE] Desconectado")
            return

        port = self.port_var.get()
        if not port:
            messagebox.showwarning("Serie", "Seleccioná un puerto COM.")
            return

        try:
            baud = int(self.baud_var.get())
            self.ser = serial.Serial(port=port, baudrate=baud, timeout=0.02)
            self.connect_button.configure(
                text="DESCONECTAR", fg_color="#DC2626", hover_color="#991B1B"
            )
            self.serial_status_var.set(f"● Conectado: {port} @ {baud}")
            self.lbl_serial_status.configure(text_color="#10B981")
            self.log_terminal(f"[SERIE] Conectado a {port} @ {baud}")
            self.serial_reader_running = True
        except Exception as e:
            self.ser = None
            self.serial_status_var.set("● Desconectado")
            self.lbl_serial_status.configure(text_color="#EF4444")
            messagebox.showerror("Error de conexión", str(e))

    def read_serial(self):
        if self.is_serial_connected():
            try:
                while self.ser.in_waiting:
                    raw = self.ser.readline()
                    if raw:
                        text = raw.decode("utf-8", errors="replace").rstrip("")
                        if text:
                            self.log_terminal(text)
                            # Al recibir la respuesta "G99", marcamos como enviado el vector de datos y evaluamos
                            if "G99" in text.upper():
                                self.g99_sent = True
                                self.update_start_button_state()
            except Exception as e:
                self.log_terminal(f"[ERROR RX] {e}")
        self.after(30, self.read_serial)

    def send_serial(self, command):
        command = command.strip()
        if not command:
            return False
        if not self.is_serial_connected():
            messagebox.showwarning("Serie", "No hay una conexión serie activa.")
            return False
        try:
            self.ser.write((command + "\n").encode("ascii"))
            self.ser.flush()
            return True
        except Exception as e:
            self.log_terminal(f"[ERROR TX] {e}")
            return False

    def send_manual(self):
        command = self.command_var.get().strip()
        if self.send_serial(command):
            self.command_var.set("")

    def send_rod_diameter(self):
        val = self.rod_diameter_var.get()
        if self.send_serial(f"G5 {val}"):
            self.position_sent = True  # NUEVO: Marca la bandera como verdadera tras el envío exitoso
            self.update_start_button_state()  # Re-evalúa el estado del botón INICIO

    def send_inicio(self):
        self.send_serial("INICIO")

    def log_terminal(self, text):
        if hasattr(self, "terminal"):
            self.terminal.insert("end", text)
            self.terminal.see("end")

    def clear_terminal(self):
        self.terminal.delete("1.0", "end")

    @staticmethod
    def fmt(value):
        value = float(value)
        return (
            str(int(value))
            if value.is_integer()
            else f"{value:.4f}".rstrip("0").rstrip(".")
        )

    def linear_distance(self):
        value = float(self.linear_distance_var.get())
        if value <= 0:
            raise ValueError("La distancia debe ser mayor que 0.")
        return value

    def y_degrees(self):
        value = float(self.y_degrees_var.get().replace("°", ""))
        if value <= 0:
            raise ValueError("Los grados deben ser mayores que 0.")
        return value

    def move_linear(self, axis, positive):
        try:
            distance = self.linear_distance()
        except ValueError as e:
            messagebox.showwarning("Movimiento", str(e))
            return
        command = "G1" if positive else "G2"
        self.send_serial(f"{command} {axis}{self.fmt(distance)}")

    def home_general(self):
        self.send_serial("G0")

    def home_axis(self, axis):
        self.send_serial(f"G0 {axis}")

    def rotate_y(self, clockwise):
        try:
            degrees = self.y_degrees()
        except ValueError as e:
            messagebox.showwarning("Movimiento Y", str(e))
            return
        command = "G1" if clockwise else "G2"
        self.send_serial(f"{command} Y{self.fmt(degrees)}")

    def toggle_motor(self):
        new_state = not self.motor_on
        command = "S1 ON" if new_state else "S1 OFF"
        if self.send_serial(command):
            self.motor_on = new_state
            if self.motor_on:
                self.motor_button.configure(
                    text="MOTOR AUXILIAR: ON",
                    fg_color="#10B981",
                    hover_color="#059669",
                )
            else:
                self.motor_button.configure(
                    text="MOTOR AUXILIAR: OFF",
                    fg_color=COLOR_BTN_AUX_OFF,
                    hover_color=COLOR_BTN_AUX_OFF_HOVER,
                )

    def close(self):
        self.serial_reader_running = False
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
        self.destroy()


if __name__ == "__main__":
    app = App()
    app.mainloop()