# Troubleshooting

## macOS `_tkinter` errors for the simulator
Homebrew Python often lacks the Tk frameworks used by the simulator GUI. Use the project Conda env and ensure it is first in `PATH`.

1) Open a fresh terminal and activate the env:
```
conda activate aquaguard-c
```
2) Reinstall Tk inside the env (harmless if already present):
```
conda install -c conda-forge tk
```
3) Verify the Python interpreter comes from Conda, not Homebrew:
```
which python
python -c "import sys; print(sys.executable)"
```
If these point to `/opt/homebrew` or `/usr/local`, prepend Conda to PATH in your shell rc file:
```
export PATH="$HOME/miniconda3/bin:$PATH"   # or $HOME/mambaforge/bin
```
Restart the terminal, reactivate the env, then launch the simulator:
```
python simulator_py/gui_simulator.py
```

Quick Tkinter check:
```
python - <<'PY'
import tkinter as tk
print("Tk version:", tk.TkVersion)
root = tk.Tk(); root.withdraw(); root.destroy()
print("Tkinter is OK")
PY
```

## Common symptoms
| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `ModuleNotFoundError: No module named '_tkinter'` | Using Homebrew/system Python or missing Tk | Activate env, `conda install -c conda-forge tk`, ensure Conda Python in PATH |
| `ImportError: No module named tkinter` | Tk package not available | Same as above |
| Tk GUI blank/crashes | Mixed Homebrew + Conda libraries | Put Conda first in PATH, reopen terminal, reactivate env |
| `conda activate` works but Python is still Homebrew | Shell PATH overrides | Remove Homebrew PATH/aliases before Conda line |

Still stuck? Recreate the env (`conda env remove -n aquaguard-c` then re-run `conda env create -f environment.yml`) and retry.
