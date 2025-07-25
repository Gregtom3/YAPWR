import numpy as np
import matplotlib.pyplot as plt
plt.style.use('science')
import matplotlib as mpl
from matplotlib.patches import Rectangle

CAPSIZE = 1

SYS_COLOR_MAP = {
    # detector / reconstruction
    "binMigration"        : "#1f77b4",   # muted blue
    "baryonContamination" : "#ff7f0e",   # orange
    "particleMisID"       : "#2ca02c",   # green
    "sidebandRegion"      : "#d62728",   # red
    "purityBinning"       : "#9467bd",   # violet‑purple

    # normalization sub‑sources
    "beamPolarization"    : "#8c564b",   # brown
    "nonDisElectrons"     : "#e377c2",   # pink
    "radiativeCorrections": "#7f7f7f",   # grey
}

# fallback colour (should rarely be used once map is complete)
DEFAULT_SYS_COLOR = "#17becf"            # teal

pw_label_dict = {
    (2,1,1): "$G_{1,OT}^{\\perp}$",
    (2,2,1): "$G_{1,LT}^{\\perp}$",
    (2,2,2): "$G_{1,TT}^{\\perp}$",
    (3,1,-1): "$H_{1,OT}^{\\perp}$",
    (3,1,1):  "$H_{1,OT}^{\\sphericalangle}$",
    (3,2,-2): "$H_{1,TT}^{\\perp}$",
    (3,2,-1): "$H_{1,LT}^{\\perp}$",
    (3,2,1):  "$H_{1,LT}^{\\sphericalangle}$",
    (3,2,2):  "$H_{1,TT}^{\\sphericalangle}$"
}

AXIS_LABELS = {
    'Mh': r'$M_{h}\ (\mathrm{GeV})$',
    'x':  r'$x$',
    'Q2': r'$Q^{2}\ (\mathrm{GeV}^{2})$',
}

# default marker/line colors per dipion pair
DEFAULT_COLORS = {
    "piplus_pi0":      "blue",
    "piplus_piminus":  "black",
    "piminus_pi0":     "red",
    "piplus_piplus":   "green",
    "piminus_piminus": "red",
}

PAIR_LABEL = {
    "piplus_pi0": "$\pi^{+}\pi^{0}$",
    "piplus_piplus": "$\pi^{+}\pi^{+}$",
    "piplus_piminus": "$\pi^{+}\pi^{-}$",
    "piminus_pi0": "$\pi^{-}\pi^{0}$",
    "piminus_piminus": "$\pi^{-}\pi^{-}$"
}

PAIR_MARKER = {
    "piplus_pi0": "s",
    "piplus_piplus": "^",
    "piplus_piminus": "o",
    "piminus_pi0": "p",
    "piminus_piminus": "v"
}

def fetchAx(yamlData,
            series_specs,
            twist, L, M,
            bin_var='Mh',
            ax=None,
            xlim=None, ylim=None,
            grid=False,
            show_lm=True,
            show_pw_label=True,
            show_sys_band=False):
    """
    Plot multiple A ± total_error series on one Axes.  
    series_specs : list of dicts, each with:
        - 'pair'         : str, pionPair to plot
        - 'label'        : str, legend label
        - 'markerstyle'  : matplotlib fmt (e.g. 'o', 's')
        - 'markersize'   : float
        - 'markercolor'  : color
        - 'linecolor'    : color
    """
    recs = yamlData
    if ax is None:
        fig, ax = plt.subplots()
    else:
        fig = ax.figure

    # precompute half‐width for sys‐band if needed
    if show_sys_band:
        dpi     = fig.dpi
        cap_pts = 1.5*CAPSIZE
        cap_pix = cap_pts * dpi / 72.0
        inv     = ax.transData.inverted()
        origin  = inv.transform((0, 0))
        right   = inv.transform((cap_pix, 0))
        half_w  = right[0] - origin[0]

    for spec in series_specs:
        pair = spec['pair']
        # collect x, A, stat+sys, and sys‐only
        x_vals, A_vals, err_vals, sys_vals = [], [], [], []
        for rec in recs:
            if (rec.get('pionPair') == pair
                and rec.get('twist') == twist
                and rec.get('L')     == L
                and rec.get('M')     == M):
                x = rec.get(bin_var)
                if x is None:
                    raise KeyError(f"Record for {pair} missing '{bin_var}'")
                x_vals .append(x)
                A_vals .append(rec['A'])
                # total error
                err_vals.append(np.hypot(rec['sStat'], rec['sSys']))
                sys_vals.append(rec['sSys'])

        if not x_vals:
            raise ValueError(f"No records for {pair}, twist={twist}, L={L}, M={M}")

        # sort
        order    = np.argsort(x_vals)
        x_vals   = np.array(x_vals)[order]
        A_vals   = np.array(A_vals)[order]
        err_vals = np.array(err_vals)[order]
        sys_vals = np.array(sys_vals)[order]

        # formatting
        mstyle = spec.get('markerstyle', PAIR_MARKER[pair])
        msize  = spec.get('markersize', None)
        mcolor = spec.get('markercolor',
                          DEFAULT_COLORS.get(pair, 'black'))
        lcolor = spec.get('linecolor',
                          DEFAULT_COLORS.get(pair, mcolor))
        label  = spec.get('label', PAIR_LABEL[pair])
        
        eb_kwargs = dict(
            x = x_vals,
            y = A_vals,
            yerr = err_vals,
            fmt = mstyle,
            color = lcolor,
            capsize = CAPSIZE,
            label = label,
        )
        if msize is not None:
            eb_kwargs['markersize'] = msize
        else:
            eb_kwargs['markersize'] = 3 
        # fill marker face
        eb_kwargs['markerfacecolor'] = mcolor
        eb_kwargs['markeredgecolor'] = mcolor

        ax.errorbar(**eb_kwargs)

        # sys‐error band
        if show_sys_band:
            for x, y, sys in zip(x_vals, A_vals, sys_vals):
                rect = Rectangle(
                    (x - half_w, y - sys),
                    2 * half_w,
                    2 * sys,
                    facecolor = mcolor,
                    alpha     = 0.3,
                    edgecolor = mcolor,
                    linewidth = 1,
                    hatch     = "//"
                )
                ax.add_patch(rect)

    # finalize axes
    xlabel = AXIS_LABELS.get(bin_var, bin_var)
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(f"$A_{{\\mathrm{{LU}}}}^{{|{L},{M}\\rangle}}$", fontsize=12)

    if show_lm:
        ax.text(0.05, 0.95, f"$|{L},{M}\\rangle$",
                transform=ax.transAxes, va='top', ha='left')
    if show_pw_label:
        pw = pw_label_dict.get((twist, L, M), '')
        if pw:
            ax.text(0.95, 0.95, pw,
                    transform=ax.transAxes, va='top', ha='right')

    ax.axhline(0, linestyle='--', color='gray', alpha=0.5)
    ax.grid(grid)
    if xlim is not None: ax.set_xlim(xlim)
    if ylim is not None: ax.set_ylim(ylim)

    ax.legend()
    return ax

def plotSysFig(yamlData,
               pair, twist, L, M,
               bin_var='Mh',
               ax=None,
               xlim=None, ylim=None,
               grid=False,
               show_lm=True,
               show_pw_label=True,
               show_sys_band=False,
               bar_alpha=0.85):

    TOP_XAXIS_FONT_SIZE = 18
    TOP_YAXIS_FONT_SIZE = 18
    BOT_XAXIS_FONT_SIZE = 8
    BOT_YAXIS_FONT_SIZE = 18
    LM_FONT_SIZE = 15
    DIFF_FONT_SIZE = 15
    
    def _collect_sys_arrays(recs, pair, twist, L, M, bin_var):
        x_vals, sys_vals = [], []
        sys_by_src = {}
    
        for rec in recs:
            if (rec.get('pionPair') == pair
                and rec.get('twist') == twist
                and rec.get('L')     == L
                and rec.get('M')     == M):
                x = rec.get(bin_var)
                if x is None:
                    raise KeyError("Record missing bin variable '{}'".format(bin_var))
                x_vals.append(x)
                sys_vals.append(float(rec['sSys']))
    
                # flatten "systematics" -> abs errors
                for src, item in rec['systematics'].items():
                    if isinstance(item, list):
                        # [rel, abs]
                        sys_by_src.setdefault(src, []).append(float(item[1]))
                    elif isinstance(item, dict):
                        for subsrc, subitem in item.items():
                            sys_by_src.setdefault(subsrc, []).append(float(subitem[1]))
                    else:
                        raise TypeError("Unexpected format for '{}'".format(src))
    
        if not x_vals:
            raise ValueError("No records for {}, twist={}, L={}, M={}".format(pair, twist, L, M))
    
        order = np.argsort(x_vals)
        x_vals   = np.asarray(x_vals)[order]
        sys_vals = np.asarray(sys_vals)[order]
        for src in sys_by_src:
            sys_by_src[src] = np.asarray(sys_by_src[src])[order]
    
        return x_vals, sys_vals, sys_by_src
    
    """
    One pair, two panels: top = A +- total error, bottom = stacked sys breakdown.
    Uses fetchAx() to draw the top panel to save code.
    """

    # records list (no more 'records' wrapper)
    recs = yamlData

    # make figure/axes (top + bottom)
    fig, (ax_top, ax_bot) = plt.subplots(
        2, 1,
        figsize=(4, 5),
        dpi=200,
        gridspec_kw=dict(height_ratios=[3, 2])
    )

    # ---------------- top panel via fetchAx ----------------
    spec = {
        'pair'        : pair,
        'markerstyle' : 'o',
        'markersize'  : 3,
        # leave colors to your defaults
    }
    fetchAx(recs,
            [spec],
            twist, L, M,
            bin_var=bin_var,
            ax=ax_top,
            xlim=xlim, ylim=ylim,
            grid=grid,
            show_lm=show_lm,
            show_pw_label=show_pw_label,
            show_sys_band=show_sys_band)

    # ensure horizontal zero line (fetchAx already does it, but harmless)
    ax_top.axhline(0, linestyle='--', color='gray', alpha=0.5)

    # ---------------- bottom panel: stacked sys bars ----------------
    x_vals, sys_vals, sys_by_src = _collect_sys_arrays(recs, pair, twist, L, M, bin_var)

    bar_x = np.arange(len(x_vals))
    cum = np.zeros_like(x_vals, dtype=float)

    # deterministic color choice
    prop_cycle = plt.rcParams['axes.prop_cycle'].by_key()['color']
    colors = {}
    for i, src in enumerate(sys_by_src.keys()):
        colors[src] = SYS_COLOR_MAP.get(src, DEFAULT_SYS_COLOR if i >= len(prop_cycle) else prop_cycle[i])

    for src, vals in sys_by_src.items():
        ax_bot.bar(bar_x, vals, bottom=cum, width=0.8,
                   label=src, color=colors[src], alpha=bar_alpha)
        cum += vals

    xlabel = AXIS_LABELS.get(bin_var, bin_var)
    ax_bot.set_ylabel(r'$\Delta A$', fontsize=BOT_YAXIS_FONT_SIZE)
    ax_bot.set_ylim(0, ax_bot.get_ylim()[1]*2)
    ax_bot.grid(axis='y', linestyle=':', alpha=0.4)

    # nice tick labels
    ax_bot.set_xticks(bar_x)
    base_label = xlabel.split('(')[0] if '(' in xlabel else xlabel
    if '$' in base_label:
        base_label += '$'
    ax_bot.set_xticklabels([f"{base_label}={x:.3g}" for x in x_vals],
                           rotation=45, ha='right', fontsize=BOT_XAXIS_FONT_SIZE)

    ax_bot.legend(ncol=2, fontsize=6, frameon=False)

    fig.tight_layout()
    return fig, (ax_top, ax_bot)


def plot_twist2_grid(yamlData,
                     pair,
                     *,
                     bin_var='Mh',
                     xlim=None,
                     ylim=None,
                     grid=False,
                     show_lm=True,
                     show_pw_label=True,
                     show_sys_band=False,
                     figsize=(6.4, 5.0)):
    """
    Quick‑view panel for twist 2.

    Layout (rows = L, cols = M):
        ┌───────────┬───────────┐
        │ (1,1)     │  ✗ (1,2)  │   ✗ ⇒ axis removed
        ├───────────┼───────────┤
        │ (2,1)     │  (2,2)    │
        └───────────┴───────────┘

    Returns
    -------
    matplotlib.figure.Figure
    """

    # create 2×2 canvas
    fig, axs = plt.subplots(2, 2, figsize=figsize, sharex=True, sharey=True)

    
    # map each (L,M) combo to the corresponding axes object
    grid_map = {
        (1, 1): axs[0, 0],
        (1, 2): axs[0, 1],
        (2, 1): axs[1, 0],
        (2, 2): axs[1, 1],
    }

    # loop over grid positions
    for (L, M), ax in grid_map.items():
        if (L, M) == (1, 2):
            # remove the unused (1,2) subplot
            fig.delaxes(ax)
            continue

        fetchAx(
            yamlData,
            pair,
            2,
            L,
            M,
            bin_var=bin_var,
            ax=ax,
            xlim=xlim,
            ylim=ylim,
            grid=grid,
            show_lm=show_lm,
            show_pw_label=show_pw_label,
            show_sys_band=show_sys_band,
        )

        if (M==1):
            ax.set_ylabel(f"$A_{{\mathrm{{LU}}}}^{{|{L},m\\rangle}}$",fontsize=12)
        else:
            ax.set_ylabel("")
    fig.subplots_adjust(wspace=0, hspace=0)
    
    return fig


# ──────────────────────────────────────────────────────────────
#  Twist‑3 grid (2 × 4) :  L = 1,2  ;  M = −2,−1,1,2
#  * Deletes (1,−2) and (1,2) axes
#  * Keeps y‑axis only on (1,−1)  and (2,−2)
# ──────────────────────────────────────────────────────────────
def plot_twist3_grid(yamlData,
                     pair,
                     *,
                     bin_var='Mh',
                     xlim=None,
                     ylim=None,
                     grid=False,
                     show_lm=True,
                     show_pw_label=True,
                     show_sys_band=False,
                     figsize=(9.0, 4.5)):
    if xlim is None and bin_var == 'Mh':
        xlim = (0, None)          # example tweak – feel free to drop

    # build full 2×4 canvas
    fig, axs = plt.subplots(2, 4, figsize=figsize, sharex=True, sharey=True)

    # delete missing twist‑3 sub‑waves in top row
    fig.delaxes(axs[0, 0])   # L=1, M=−2
    fig.delaxes(axs[0, 3])   # L=1, M= 2

    # surviving axes → dict keyed by (L,M)
    grid_map = {
        (1, -1): axs[0, 1],
        (1,  1): axs[0, 2],
        (2, -2): axs[1, 0],
        (2, -1): axs[1, 1],
        (2,  1): axs[1, 2],
        (2,  2): axs[1, 3],
    }

    for (L, M), ax in grid_map.items():
        fetchAx(
            yamlData,
            pair,
            3,          # twist = 3
            L,
            M,
            bin_var=bin_var,
            ax=ax,
            xlim=xlim,
            ylim=ylim,
            grid=grid,
            show_lm=show_lm,
            show_pw_label=show_pw_label,
            show_sys_band=show_sys_band,
        )

        # y‑axis only on (1,−1) and (2,−2)
        if not ((L, M) in [(1, -1), (2, -2)]):
            ax.set_ylabel('')

    fig.subplots_adjust(wspace=0, hspace=0)
    return fig


# ──────────────────────────────────────────────────────────────
#  Convenience dispatcher
# ──────────────────────────────────────────────────────────────
def plot_grid(yamlData,
              pair,
              twist,
              **kwargs):
    """
    Wrapper that chooses twist‑specific layout.

    Parameters
    ----------
    yamlData : dict
    pair     : str
    twist    : int   (2 or 3)
    **kwargs : forwarded to the underlying grid helper
    """
    if twist == 2:
        return plot_twist2_grid(yamlData, pair,
                                **kwargs)
    elif twist == 3:
        return plot_twist3_grid(yamlData, pair,
                                **kwargs)
    else:
        raise ValueError("Only twist 2 or 3 supported for grid plots")

