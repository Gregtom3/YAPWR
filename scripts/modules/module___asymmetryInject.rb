require_relative 'module_runner'
require 'yaml'
require 'fileutils'

class AsymmetryInjectionPWRunner < ModuleRunner
  # ------------------------------------------------------------------
  # constants / simple hooks
  # ------------------------------------------------------------------
  RUN_TO_MC = {
    'Fall2018_RGA_inbending'          => 'MC_RGA_inbending',
    'Spring2019_RGA_inbending'        => 'MC_RGA_inbending',
    'Fall2018_RGA_outbending'         => 'MC_RGA_outbending',
    'Fall2018Spring2019_RGA_inbending'=> 'MC_RGA_inbending'
  }.freeze

  def module_key          ; 'asymmetryInjectionPW' end
  def slurm_job_name(tag) ; "inj_#{tag}"            end
  def slurm_directives    ; { time: '24:00:00', mem_per_cpu: '4000', cpus: 1 } end

  # we do **not** require a *data*‑side filtered TFile – the macro only
  # needs the MC version.
  def needs_filtered_file? ; false end

  # ignore MC leaves; process only data
  def keep_leaf?(tag, _info_path)
    !tag.start_with?('MC_')
  end

  # ------------------------------------------------------------------
  # main per‑leaf logic
  # ------------------------------------------------------------------
  def process_leaf(ctx)
    pair        = File.basename(File.dirname(ctx[:leaf_dir]))
    run_version = ctx[:tag]

    mc_tag = RUN_TO_MC[run_version]
    unless mc_tag
      warn "[#{module_key}][#{run_version}] No MC mapping → skipping"
      return
    end

    # ---------- locate MC leaf ------------------------------------------------
    mc_leaf_dir  = File.join(ctx[:cfg_dir], pair, mc_tag)
    mc_info_path = File.join(mc_leaf_dir, 'tree_info.yaml')
    unless File.exist?(mc_info_path)
      warn "[#{module_key}] MC tree_info.yaml missing: #{mc_info_path}"
      return
    end

    mc_info           = YAML.load_file(mc_info_path)
    mc_orig_tfile     = mc_info.fetch('tfile')
    filtered_mc_tfile = File.join(mc_leaf_dir, File.basename(mc_orig_tfile))
    unless File.exist?(filtered_mc_tfile)
      warn "[#{module_key}] filtered MC file not found: #{filtered_mc_tfile}"
      return
    end

    # ---------- locate data asymmetry YAML ------------------------------------
    data_asym_yaml = File.join(ctx[:leaf_dir],
                               'module-out___asymmetryPW',
                               'asymmetry_results.yaml')
    unless File.exist?(data_asym_yaml)
      alt = File.join(ctx[:leaf_dir],
                      'module-out___asymmetryPW',
                      'asymmetryPW.yaml')
      if File.exist?(alt)
        data_asym_yaml = alt
      else
        warn "[#{module_key}] asymmetry YAML not found in data leaf"
        return
      end
    end

    # ---------- build & run ----------------------------------------------------
    outdir = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)

    cmd = build_root_cmd(ctx.merge(
      pair:               pair,
      outdir:             outdir,
      filtered_MC_tfile:  filtered_mc_tfile,
      dataAsymYamlFile:   data_asym_yaml
    ))

    run_job(run_version, outdir, cmd)
  end

  # ------------------------------------------------------------------
  # macro call string
  # ------------------------------------------------------------------
  def macro_call(ctx)
    %Q{'src/modules/injectAsymmetry.C("#{ctx[:filtered_MC_tfile]}",
"#{ctx[:tree_name]}","#{ctx[:pair]}","#{ctx[:outdir]}","#{ctx[:dataAsymYamlFile]}")'}
  end
end

AsymmetryInjectionPWRunner.run!
