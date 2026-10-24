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
        self.position_sent = False
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
        """Evalúa si se cumplen ambas condiciones para habilitar el botón INICIO."""
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
            "1. Conexión Serie: Selecciona el Puerto COM de tu dispositivo (ej. Arduino). Usa el botón 'Actualizar' si no aparece. Selecciona el Baudrate (por defecto 115200) y presiona CONECTAR.\n"
            "2. Cargar DXF: Haz clic en el botón 'Cargar DXF' y selecciona tu archivo .dxf. El sistema analizará las limitaciones mecánicas.\n"
            "3. Centro y Referencia: Haz clic en 'Detectar centro' para encontrar automáticamente la entidad CIRCLE más grande. Si no la hay, ingresa las coordenadas X e Y manualmente.\n"
            "4. Configurar el Análisis: Selecciona la Resolución angular deseada (p. ej., 1.0°) y el sentido de barrido (Antihorario u Horario).\n"
            "5. Ejecutar: Haz clic en 'ANALIZAR 360°'. La tabla se llenará con los pares de Ángulo (°) y Distancia (mm).\n"
            "6. Enviar DATOS o Exportar: Puedes transmitir las distancias directamente usando 'ENVIAR DATOS' o guardar los resultados limpios en un archivo plano mediante 'Exportar a TXT'.\n\n"
            "Pestaña: Control y Terminal\n"
            "7. Enviar Posición: Selecciona el Diámetro (mm) y el Largo (cm) de la varilla en los desplegables. Presiona 'Enviar posicion' para transmitir el comando (ej. G5 26 30).\n"
            "8. Inicio de Trabajo: El botón 'INICIO' se habilitará automáticamente una vez enviado el comando de análisis (G99) Y presionado 'Enviar posicion'.\n"
            "------------------------ FUNCIONES ADICIONALES ------------------------\n"
            "1. Panel de Control Manual: Configura las magnitudes de paso para los ejes X/Z e Y. Utiliza la cruceta para mover los ejes o volver a la posición HOME.\n"
            "2. Motor Auxiliar: Activa o desactiva la sierra (S1 ON / S1 OFF).\n"
            "3. Terminal Serie: Monitoriza respuestas en tiempo real o envía comandos manuales."
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
        self.res_var = ctk.StringVar(value="5.0°")
        ctk.CTkOptionMenu(
            cfg_inner,
            variable=self.res_var,
            values=[f"{i/2:0.1f}°" for i in range(1, 11)],
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

        action_frame.columnconfigure(0, weight=6)
        action_frame.columnconfigure(1, weight=2)
        action_frame.columnconfigure(2, weight=2)

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
        self.btn_send_g99.grid(row=0, column=1, sticky="ew", padx=3)
        self.set_btn_state(self.btn_send_g99, False)

        # Botón Exportar TXT movido a esta pestaña
        ctk.CTkButton(
            action_frame,
            text="Exportar a TXT",
            corner_radius=8,
            height=38,
            font=("Segoe UI", 13, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.export_dxf_txt,
        ).grid(row=0, column=2, sticky="ew", padx=(3, 0))

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
        # CONTROL: Diámetro (Fila 1) + Largo (Fila 2) + Botón Enviar (A la derecha ocupando ambas filas)
        # ----------------------------------------------------
        rod_grid = ctk.CTkFrame(container, fg_color="transparent")
        rod_grid.pack(pady=(0, 10))

        # Diámetro varilla (Fila 0, Columna 0 y 1)
        ctk.CTkLabel(
            rod_grid, text="Diametro varilla (mm):", font=("Segoe UI", 13)
        ).grid(row=0, column=0, padx=4, pady=2, sticky="e")

        self.rod_diameter_var = ctk.StringVar(value="26")
        ctk.CTkOptionMenu(
            rod_grid,
            variable=self.rod_diameter_var,
            values=[str(i) for i in range(10, 31)],
            width=70,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        ).grid(row=0, column=1, padx=4, pady=2, sticky="w")

        # Largo varilla (Fila 1, Columna 0 y 1)
        ctk.CTkLabel(
            rod_grid, text="Largo varilla (cm):", font=("Segoe UI", 13)
        ).grid(row=1, column=0, padx=4, pady=2, sticky="e")

        self.rod_length_var = ctk.StringVar(value="30")
        ctk.CTkOptionMenu(
            rod_grid,
            variable=self.rod_length_var,
            values=["10", "20", "30", "40", "50"],
            width=70,
            corner_radius=6,
            font=("Segoe UI", 13),
            button_color=COLOR_BTN_ACTION,
            fg_color=COLOR_FG_DROPDOWN,
            text_color=COLOR_TEXT_DROPDOWN,
            button_hover_color=COLOR_BTN_HOVER
        ).grid(row=1, column=1, padx=4, pady=2, sticky="w")

        # Botón "Enviar posicion" a la derecha abarcando ambas filas (rowspan=2)
        ctk.CTkButton(
            rod_grid,
            text="Enviar posicion",
            corner_radius=6,
            font=("Segoe UI", 12, "bold"),
            fg_color=COLOR_BTN_ACTION,
            hover_color=COLOR_BTN_HOVER,
            command=self.send_rod_diameter,
        ).grid(row=0, column=2, rowspan=2, padx=(10, 4), pady=2, sticky="nsew")

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
    # MÉTODOS DE PROCESAMIENTO CAD Y DXF
    # ============================================================
    def load_dxf(self):
        filename = filedialog.askopenfilename(
            filetypes=[("Archivos DXF", "*.dxf"), ("Todos los archivos", "*.*")]
        )
        if not filename:
            return

        try:
            self.doc = ezdxf.readfile(filename)
            msp = self.doc.modelspace()
            self.entities = list(msp)
            self.file_var.set(filename)
            self.status_var.set(f"DXF cargado: {len(self.entities)} entidades.")
            self.detect_circle_center()
            self.validar_silueta()
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo cargar el archivo DXF:\n{e}")

    def detect_circle_center(self):
        if not self.entities:
            return

        circles = [e for e in self.entities if e.dxftype() == "CIRCLE"]
        if circles:
            largest = max(circles, key=lambda c: float(c.dxf.radius))
            self.reference_circle = largest
            cx, cy, _ = largest.dxf.center
            self.x_var.set(f"{cx:.3f}")
            self.y_var.set(f"{cy:.3f}")
            self.circle_info_var.set(
                f"Círculo de referencia: Centro ({cx:.2f}, {cy:.2f}) | Radio = {largest.dxf.radius:.2f} mm"
            )
        else:
            self.circle_info_var.set("Círculo de referencia: No detectado (ingrese manualmente).")

    def get_center(self):
        try:
            return float(self.x_var.get()), float(self.y_var.get())
        except ValueError:
            return 0.0, 0.0

    def build_geometry(self):
        segments = []
        for e in self.entities:
            if self.reference_circle is not None and e is self.reference_circle:
                continue
            typ = e.dxftype()
            if typ == "LINE":
                p1 = (float(e.dxf.start.x), float(e.dxf.start.y))
                p2 = (float(e.dxf.end.x), float(e.dxf.end.y))
                segments.append((p1, p2))
            elif typ in ("LWPOLYLINE", "POLYLINE"):
                if typ == "LWPOLYLINE":
                    pts = [(float(p[0]), float(p[1])) for p in e.get_points("xy")]
                else:
                    pts = [(float(v.dxf.location.x), float(v.dxf.location.y)) for v in e.vertices]
                
                for i in range(len(pts) - 1):
                    segments.append((pts[i], pts[i + 1]))
                if getattr(e, "closed", False) or getattr(e, "is_closed", False):
                    segments.append((pts[-1], pts[0]))
        return segments

    def ray_segment_intersection(self, cx, cy, dx, dy, p1, p2):
        x1, y1 = p1
        x2, y2 = p2
        sx, sy = x2 - x1, y2 - y1

        denom = dx * sy - dy * sx
        if abs(denom) < 1e-9:
            return None

        t = ((x1 - cx) * sy - (y1 - cy) * sx) / denom
        u = ((x1 - cx) * dy - (y1 - cy) * dx) / denom

        if t >= 0 and 0 <= u <= 1:
            return t
        return None

    def analyze(self):
        if not self.entities:
            messagebox.showwarning("Atención", "Por favor, carga un archivo DXF primero.")
            return

        cx, cy = self.get_center()
        segments = self.build_geometry()
        if not segments:
            messagebox.showerror("Error", "No se encontraron geometrías válidas en el DXF.")
            return

        try:
            res_val = float(self.res_var.get().replace("°", ""))
        except ValueError:
            res_val = 1.0

        steps = int(round(360.0 / res_val))
        self.results.clear()
        for item in self.tree.get_children():
            self.tree.delete(item)

        is_ccw = self.direction_var.get() == "Antihorario"

        for i in range(steps):
            angle_deg = i * res_val
            calc_angle = angle_deg if is_ccw else (360.0 - angle_deg) % 360.0
            rad = math.radians(calc_angle)
            dx, dy = math.cos(rad), math.sin(rad)

            intersecciones = []
            for p1, p2 in segments:
                t = self.ray_segment_intersection(cx, cy, dx, dy, p1, p2)
                if t is not None:
                    intersecciones.append(t)

            dist = min(intersecciones) if intersecciones else 0.0
            self.results.append((angle_deg, dist))
            self.tree.insert("", "end", values=(f"{angle_deg:.1f}", f"{dist:.3f}"))

        self.status_var.set(f"Análisis completado: {len(self.results)} puntos procesados.")
        self.set_btn_state(self.btn_send_g99, True)

    def send_g99(self):
        if not self.results:
            messagebox.showwarning("Atención", "No hay datos del análisis para enviar.")
            return

        d_str = ",".join([f"{d:.2f}" for _, d in self.results])
        cmd = f"G99 D={d_str}"
        self.send_command(cmd)
        self.g99_sent = True
        self.update_start_button_state()
        self.tabview.set(self.tab_control_name)

    def export_dxf_txt(self):
        """Exporta únicamente la cadena limpia de distancias del análisis radial a un archivo TXT."""
        if not self.results:
            messagebox.showwarning("Atención", "No se han generado datos de análisis de 360° aún.")
            return

        filepath = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Archivos de texto", "*.txt"), ("Todos los archivos", "*.*")],
            title="Exportar datos DXF a TXT"
        )
        if not filepath:
            return

        try:
            d_str = ",".join([f"{d:.2f}" for _, d in self.results])

            with open(filepath, "w", encoding="utf-8") as f:
                f.write(d_str)

            messagebox.showinfo("Exportación Exitosa", f"Datos exportados correctamente en:\n{filepath}")
            self.log_terminal(f"Datos exportados a archivo TXT: {filepath}")
        except Exception as e:
            messagebox.showerror("Error de Exportación", f"No se pudo guardar el archivo:\n{e}")

    # ============================================================
    # LÓGICA DE VALIDACIÓN GEOMÉTRICA DXF
    # ============================================================
    def validar_silueta(self):
        """Valida que la silueta del DXF cargado cumpla con las restricciones mecánicas."""
        errores = []

        closed_loops = 0
        for e in self.entities:
            if self.reference_circle is not None and e is self.reference_circle:
                continue
            typ = e.dxftype()
            if typ == "CIRCLE":
                closed_loops += 1
            elif typ in ("LWPOLYLINE", "POLYLINE"):
                if getattr(e, "closed", False) or getattr(e, "is_closed", False):
                    closed_loops += 1

        if closed_loops > 1:
            errores.append("- Contiene agujeros internos o múltiples geometrías cerradas independientes.")

        segments = self.build_geometry()
        if not segments:
            return True

        cx, cy = self.get_center()

        sombras_detectadas = False
        perfil_picos = []

        for deg in range(360):
            rad = math.radians(deg)
            dx, dy = math.cos(rad), math.sin(rad)
            intersecciones = []
            for p1, p2 in segments:
                t = self.ray_segment_intersection(cx, cy, dx, dy, p1, p2)
                if t is not None:
                    intersecciones.append(t)

            intersecciones.sort()
            unique_intersections = []
            for val in intersecciones:
                if not unique_intersections or abs(val - unique_intersections[-1]) > 1e-3:
                    unique_intersections.append(val)

            if len(unique_intersections) > 1:
                sombras_detectadas = True

            if unique_intersections:
                min_t = min(unique_intersections)
                perfil_picos.append((deg, min_t, math.cos(rad) * min_t + cx, math.sin(rad) * min_t + cy))

        if sombras_detectadas:
            errores.append("- Avanza sobre zonas de sombra (reingresos o superficies no alcanzables radialmente).")

        for e in self.entities:
            if self.reference_circle is not None and e is self.reference_circle:
                continue
            typ = e.dxftype()
            if typ == "ARC":
                r = float(e.dxf.radius)
                if r < 2.0:
                    errores.append(f"- Presenta arcos cóncavos con radio menor a 2.0 mm (Encontrado: {r:.2f} mm).")
                    break

        angulo_excedido = False
        for e in self.entities:
            if self.reference_circle is not None and e is self.reference_circle:
                continue
            typ = e.dxftype()
            if typ in ("LWPOLYLINE", "POLYLINE"):
                if typ == "LWPOLYLINE":
                    pts = [(float(p[0]), float(p[1])) for p in e.get_points("xy")]
                else:
                    pts = [(float(v.dxf.location.x), float(v.dxf.location.y)) for v in e.vertices]

                if len(pts) >= 3:
                    for i in range(len(pts) - 1):
                        p_prev = pts[i - 1]
                        p_curr = pts[i]
                        p_next = pts[i + 1 if i + 1 < len(pts) else 0]

                        v1 = (p_prev[0] - p_curr[0], p_prev[1] - p_curr[1])
                        v2 = (p_next[0] - p_curr[0], p_next[1] - p_curr[1])

                        dot = v1[0] * v2[0] + v1[1] * v2[1]
                        mag1 = math.hypot(v1[0], v1[1])
                        mag2 = math.hypot(v2[0], v2[1])

                        if mag1 * mag2 > 0:
                            cos_a = max(-1.0, min(1.0, dot / (mag1 * mag2)))
                            angle_deg = math.degrees(math.acos(cos_a))
                            deflection = 180.0 - angle_deg
                            if deflection > 35.0:
                                angulo_excedido = True
                                break
            if angulo_excedido:
                errores.append("- Presenta ángulos cóncavos pronunciados que superan los 35°.")
                break

        if len(perfil_picos) >= 3:
            picos_local = []
            for i in range(len(perfil_picos)):
                prev_d = perfil_picos[i - 1][1]
                curr_d = perfil_picos[i][1]
                next_d = perfil_picos[(i + 1) % len(perfil_picos)][1]

                if curr_d > prev_d and curr_d > next_d:
                    picos_local.append(perfil_picos[i])

            separacion_insuficiente = False
            for i in range(len(picos_local) - 1):
                pk1 = picos_local[i]
                pk2 = picos_local[i + 1]
                dist_picos = math.hypot(pk1[2] - pk2[2], pk1[3] - pk2[3])
                if dist_picos < 1.0:
                    separacion_insuficiente = True
                    break

            if separacion_insuficiente:
                errores.append("- La separación entre crestas/picos adyacentes es menor a 1.0 mm.")

        if errores:
            msg = "⚠️ LA SILUETA ADJUNTADA NO SE PUEDE REALIZAR.\n\n"
            msg += "Motivos / Restricciones no cumplidas:\n"
            msg += "\n".join(errores)
            messagebox.showwarning("Aviso de Restricción Mecánica", msg)

    # ============================================================
    # COMUNICACIÓN SERIE Y CONTROL MANUAL
    # ============================================================
    def refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo.configure(values=ports)
        if ports:
            self.port_var.set(ports[0])
        else:
            self.port_var.set("")

    def toggle_serial(self):
        if self.ser is None or not self.ser.is_open:
            port = self.port_var.get()
            baud = self.baud_var.get()
            if not port:
                messagebox.showerror("Error", "No se ha seleccionado ningún puerto COM.")
                return
            try:
                self.ser = serial.Serial(port, int(baud), timeout=0.1)
                self.serial_status_var.set("● Conectado")
                self.lbl_serial_status.configure(text_color="#22C55E")
                self.connect_button.configure(text="DESCONECTAR")
                self.log_terminal(f"Conectado exitosamente a {port} @ {baud} bps.")
            except Exception as e:
                messagebox.showerror("Error de Conexión", f"No se pudo abrir el puerto:\n{e}")
        else:
            self.close_serial()

    def close_serial(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.ser = None
        self.serial_status_var.set("● Desconectado")
        self.lbl_serial_status.configure(text_color="#EF4444")
        self.connect_button.configure(text="CONECTAR")
        self.log_terminal("Puerto serie desconectado.")

    def read_serial(self):
        if self.ser and self.ser.is_open and self.ser.in_waiting:
            try:
                data = self.ser.readline().decode("utf-8", errors="ignore").strip()
                if data:
                    self.log_terminal(f"← {data}")
            except Exception:
                pass
        self.after(30, self.read_serial)

    def send_command(self, cmd: str):
        self.log_terminal(f"→ {cmd}")
        if self.ser and self.ser.is_open:
            try:
                self.ser.write((cmd + "\n").encode("utf-8"))
            except Exception as e:
                self.log_terminal(f"Error al enviar: {e}")

    def send_manual(self):
        cmd = self.command_var.get().strip()
        if cmd:
            self.send_command(cmd)
            self.command_var.set("")

    def clear_terminal(self):
        self.terminal.delete("1.0", "end")

    def log_terminal(self, msg: str):
        self.terminal.insert("end", msg + "\n")
        self.terminal.see("end")

    def send_rod_diameter(self):
        diametro = self.rod_diameter_var.get()
        largo = self.rod_length_var.get()
        cmd = f"G5 {diametro}{largo}"
        self.send_command(cmd)
        self.position_sent = True
        self.update_start_button_state()

    def move_linear(self, axis: str, positive: bool):
        dist = float(self.linear_distance_var.get())
        sign = "" if positive else "-"
        self.send_command(f"G1 {axis}{sign}{dist}")

    def rotate_y(self, clockwise: bool):
        deg = float(self.y_degrees_var.get().replace("°", ""))
        sign = "-" if clockwise else ""
        self.send_command(f"G1 Y{sign}{deg}")

    def home_general(self):
        self.send_command("G0")

    def home_axis(self, axis: str):
        self.send_command(f"G0 {axis}")

    def toggle_motor(self):
        self.motor_on = not self.motor_on
        if self.motor_on:
            self.motor_button.configure(
                text="MOTOR SIERRA: ON",
                fg_color=COLOR_BTN_START,
                hover_color=COLOR_BTN_START_HOVER
            )
            self.send_command("S1 ON")
        else:
            self.motor_button.configure(
                text="MOTOR SIERRA: OFF",
                fg_color=COLOR_BTN_AUX_OFF,
                hover_color=COLOR_BTN_AUX_OFF_HOVER
            )
            self.send_command("S1 OFF")

    def send_inicio(self):
        self.send_command("G1 INICIO")

    def close(self):
        self.close_serial()
        self.destroy()


if __name__ == "__main__":
    app = App()
    app.mainloop()