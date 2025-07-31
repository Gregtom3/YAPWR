import os, re, glob, yaml
import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from collections import OrderedDict

def loadFromYamlPath(path):
    with open(path) as f:
        docs = list(yaml.safe_load_all(f))  # allow multi-doc

    # Flatten: accept lists or single maps
    out = []
    for d in docs:
        if d is None:
            continue
        if isinstance(d, list):
            out.extend(d)
        else:
            # if someone accidentally dumped a single map, keep it
            out.append(d)
    return out


# -- utility: grab "0007" from "..._0007.yaml"
_digit_re = re.compile(r'(\d+)(?=\.ya?ml$)', re.I)
def _trial_index(fname):
    m = _digit_re.search(fname)
    return int(m.group(1)) if m else None

def sort_cfgB(path: str):
    """
    Extracts the Mh bin edges from a path like
      …/config_config_Mh_0p6_0p68/… 
    and returns (xmin, xmax) as floats, e.g. (0.6, 0.68).
    """
    m = re.search(r'config_config_[^_]+_([^_]+)_([^/]+)', path)
    if not m:
        # no match → put it at the end
        return (float('inf'), float('inf'))
    xmin_s, xmax_s = m.group(1), m.group(2)
    xmin = float(xmin_s.replace('p', '.'))
    xmax = float(xmax_s.replace('p', '.'))
    return (xmin, xmax)

def get_bin_center_from_cfg(path: str):
    m = re.search(r'config_config_[^_]+_([^_]+)_([^/]+)', path)
    if not m:
        raise ValueError(f"Could not parse edges from path: {path!r}")
    xmin_s, xmax_s = m.group(1), m.group(2)
    xmin = float(xmin_s.replace('p', '.'))
    xmax = float(xmax_s.replace('p', '.'))
    return np.mean([xmin,xmax])
    
def b_to_twist_L_M(b):
    """
    Map the partial-wave index b to (twist, L, M) according to the pwTable:
      b:   0   1   2   3   4   5   6   7   8   9   10  11
    tw:   2   2   2   3   3   3   3   3   3   3    3    3
    L:    1   2   2   0   1   1   1   2   2   2    2    2
    M:    1   1   2   0  -1   0   1  -2  -1    0    1    2
    """
    pw_map = {
        0:  (2, 1,  1),
        1:  (2, 2,  1),
        2:  (2, 2,  2),
        3:  (3, 0,  0),
        4:  (3, 1, -1),
        5:  (3, 1,  0),
        6:  (3, 1,  1),
        7:  (3, 2, -2),
        8:  (3, 2, -1),
        9:  (3, 2,  0),
        10: (3, 2,  1),
        11: (3, 2,  2),
    }
    try:
        return pw_map[b]
    except KeyError:
        raise ValueError(f"Invalid partial-wave index b={b}, must be 0–11")
# ------------------------------------------------------------------------
# 1) Data loader – *only* IO / parsing logic lives here
# ------------------------------------------------------------------------
@dataclass(frozen=True)
class InjectionData:
    x_vals:       np.ndarray                    # 0..M-1
    true_vals:    np.ndarray                    # length M
    true_errs:    np.ndarray                    # length M
    pion_pair:    str
    L_:           int
    M_:           int
    twist_:       int
    trials:       dict[int, np.ndarray]         # trial_id -> length M
    config_paths: tuple                         # original config roots

def injection_dataloader(
        *,
        project_dir,
        pion_pair,
        run_version,
        region   = 'signal_purity_1_1',
        b_index  = 0
) -> InjectionData:
    """
    Collect "true" and injected b_k values across *all* configs.

    Directory pattern searched:
        out/<project_dir>/*/<pion_pair>/<run_version>/

    Returns
    -------
    InjectionData (frozen dataclass)
    """
    # locate true YAMLs
    true_glob = glob.glob(os.path.join(
        '../out', project_dir, '*', pion_pair, run_version,
        'module-out___asymmetryPW', 'asymmetry_results.yaml'
    ))
    true_paths = sorted(true_glob, key=sort_cfgB)
    if not true_paths:
        raise FileNotFoundError(f'No "true" YAMLs match {true_glob}')

    M        = len(true_paths)
    x_vals   = np.arange(M, dtype=float)
    true_vals = np.empty(M, dtype=float)
    true_errs = np.empty(M, dtype=float)
    trials: dict[int, list] = {}          # trial_id -> list of length M
    asymmetryYamlData = loadFromYamlPath("../out/"+project_dir+"/asymmetry_results.yaml")
    for m, true_yaml in enumerate(true_paths):
        cfg_name = os.path.basename(
                   os.path.dirname(
                   os.path.dirname(
                   os.path.dirname(
                   os.path.dirname(true_yaml)))))
        x = None
        twist,L_,M_ = b_to_twist_L_M(b_index)
        for rec in asymmetryYamlData:
            if (rec.get('pionPair') == pion_pair and rec.get('cfg') == cfg_name
                and rec.get('twist') == twist
                and rec.get('L')     == L_
                and rec.get('M')     == M_):
                x = rec
                break
        # ---- read true value -------------------------------------
        with open(true_yaml, 'r') as f:
            data = yaml.safe_load(f)
        entry = next(e for e in data['results']
                     if e['region'] == region)
        key   = f'b_{b_index}'
        x_vals[m]    = get_bin_center_from_cfg(true_yaml)
        true_vals[m] = x["A_raw"]
        true_errs[m] = np.sqrt(x["sStat"]**2+x["sSys"]**2)
        # ---- read injections -------------------------------------
        cfg_root = os.path.dirname(os.path.dirname(true_yaml))
        inj_dir  = os.path.join(cfg_root, 'module-out___asymmetryInjectionPW')
        for fpath in sorted(glob.glob(os.path.join(inj_dir, '*.yaml')), key=sort_cfgB):
            tidx = _trial_index(os.path.basename(fpath))
            if tidx is None:
                continue
            with open(fpath, 'r') as f:
                inj = yaml.safe_load(f)
            inj_entry = next((e for e in inj['results']
                              if e['region'] == region), None)
            if not inj_entry or f'b_{b_index}' not in inj_entry:
                continue
            trials.setdefault(tidx, [np.nan]*M)
            trials[tidx][m] = float(inj_entry[f'b_{b_index}'])
    # convert trial lists -> ndarray
    trials = {k: np.asarray(v, dtype=float) for k, v in trials.items()}

    return InjectionData(
        x_vals       = x_vals,
        true_vals    = true_vals,
        true_errs    = true_errs,
        pion_pair    = pion_pair,
        L_           = L_,
        M_           = M_,
        twist_       = twist,
        trials       = trials,
        config_paths = tuple(os.path.dirname(os.path.dirname(p)) for p in true_paths)
    )