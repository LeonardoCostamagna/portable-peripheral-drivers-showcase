# Doxygen Documentation

---

API documentation is fully configured and generated using Doxygen, based on inline documentation comments inside the driver header files.

## Prerequisites

To build the documentation, you need:

* **Doxygen**

## How to Generate Documentation (Command Line)

1. Open a terminal inside the driver's `docs/` folder.
2. Run Doxygen using the configuration file:

```bash
doxygen doxyfile
```

3. Open the generated HTML documentation in your browser:

```bash
# On Linux/Ubuntu
xdg-open html/index.html
```

## Alternative Method (Using Doxywizard)

If you prefer a graphical user interface (GUI):

1. Open a terminal and launch Doxywizard:

```bash
doxywizard
```

2. Go to **File > Open** and select the `doxyfile` located inside the `docs/` folder.
3. Click the **Run** button to generate the documentation.
4. Open the resulting `html/index.html` file in your web browser to view the documentation.

> **Note:** You can freely customize the `doxyfile` configuration settings to match your desired documentation layout and preferences.

---