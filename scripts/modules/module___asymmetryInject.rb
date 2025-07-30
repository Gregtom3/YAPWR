# scripts/asymmetry_injection_pw_runner.rb
require_relative 'module_runner'
require 'yaml'
require 'fileutils'

class AsymmetryInjectionPWRunner < ModuleRunner
  # ---------------------------------------------------------------
  RUN_TO_MC = {
    'Fall2018_RGA_inbending'           => 'MC_RGA_inbending',
    'Spring2019_RGA_inbending'         => 'MC_RGA_inbending',
    'Fall2018_RGA_outbending'          => 'MC_RGA_outbending',
    'Fall2018Spring2019_RGA_inbending' => 'MC_RGA_inbending'
  }.freeze

  def module_key          ; 'asymmetryInjectionPW' end
  def slurm_job_name(t)   ; "inj_#{t}"              end
  def slurm_directives    ; { time: '24:00:00', mem_per_cpu: '4000', cpus: 1 } end
  def needs_filtered_file?; false                  end     # data-side filter not required
  def keep_leaf?(tag, _)  ; !tag.start_with?('MC_') end    # data only

  # ---------------------------------------------------------------
  # CLI parsing – adds --jobs and --per-job flags
  # ---------------------------------------------------------------
  private def parse_cli(opts_hash)
    opts = OptionParser.new do |o|
      o.banner = "Usage: #{$0} [options] PROJECT_NAME [CONFIG ...]"

      o.on('--slurm', 'Submit via sbatch')               { opts_hash[:slurm] = true }
      o.on('--dependency D', String,
           'Pass "afterok:IDs" to sbatch --dependency') { |d| opts_hash[:deps] = d }
      o.on('--maxEntries N', Integer,
           'Limit max entries (positive)')              { |n| opts_hash[:max_entries] = n if n.positive? }

      # NEW:
      o.on('--jobs N', Integer,
           'Number of SLURM jobs to submit (default 10)')      { |n| opts_hash[:jobs]    = n if n.positive? }
      o.on('--per-job M', '--trials-per-job M', Integer,
           'Number of trials *inside* each job (default 10)')  { |m| opts_hash[:per_job] = m if m.positive? }

      o.on('-h', '--help', 'Show this help') { puts o; exit }
    end

    opts.parse!(ARGV)

    project   = ARGV.shift or abort opts.banner
    user_cfgs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }

    opts_hash[:max_entries] ||= -1
    opts_hash[:jobs]       ||= 4
    opts_hash[:per_job]    ||= 3
    [project, user_cfgs]
  end

  # ---------------------------------------------------------------
  # main per‑leaf logic
  # ---------------------------------------------------------------
  def process_leaf(ctx)
    run_version = ctx[:tag]
    pair        = File.basename(File.dirname(ctx[:leaf_dir]))

    # ---------- matching MC leaf ----------------------------------
    mc_tag = RUN_TO_MC[run_version]
    unless mc_tag
      warn "[#{module_key}] No MC mapping for #{run_version} → skip"
      return
    end

    mc_leaf_dir  = File.join(ctx[:cfg_dir], pair, mc_tag)
    mc_info_path = File.join(mc_leaf_dir, 'tree_info.yaml')
    unless File.exist?(mc_info_path)
      warn "[#{module_key}] Missing #{mc_info_path}"
      return
    end

    mc_info           = YAML.load_file(mc_info_path)
    mc_orig_tfile     = mc_info.fetch('tfile')
    filtered_mc_tfile = File.join(mc_leaf_dir, File.basename(mc_orig_tfile))
    unless File.exist?(filtered_mc_tfile)
      warn "[#{module_key}] Missing MC filtered file: #{filtered_mc_tfile}"
      return
    end

    # ---------- locate data asymmetry YAML ------------------------
    data_asym_yaml = File.join(ctx[:leaf_dir],
                               'module-out___asymmetryPW',
                               'asymmetry_results.yaml')
    unless File.exist?(data_asym_yaml)
      alt = File.join(ctx[:leaf_dir],
                      'module-out___asymmetryPW',
                      'asymmetryPW.yaml')
      data_asym_yaml = alt if File.exist?(alt)
    end
    unless File.exist?(data_asym_yaml)
      warn "[#{module_key}] No asymmetry YAML in data leaf"
      return
    end

    # ---------- output dir ----------------------------------------
    outdir = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)

    # -------------------------------------------------------------
    # build trials
    # -------------------------------------------------------------
    total_trials = options[:jobs] * options[:per_job]
    cmds = (0...total_trials).map do |trial|
      build_root_cmd(ctx.merge(
        pair:               pair,
        outdir:             outdir,
        filtered_MC_tfile:  filtered_mc_tfile,
        dataAsymYamlFile:   data_asym_yaml,
        trial:              trial
      ))
    end

    # slice into batches of --per-job trials and launch
    cmds.each_slice(options[:per_job]).with_index do |cmd_batch, idx|
      batch_tag = "#{run_version}_batch#{idx}"
      run_multi(batch_tag, outdir, cmd_batch)
    end
  end

  # ---------------------------------------------------------------
  # ROOT macro call (extra integer trial argument)
  # ---------------------------------------------------------------
  def macro_call(ctx)
    %Q{'src/modules/injectAsymmetry.C("#{ctx[:filtered_MC_tfile]}",
"#{ctx[:tree_name]}","#{ctx[:pair]}","#{ctx[:outdir]}","#{ctx[:dataAsymYamlFile]}",#{ctx[:trial]})'}
  end

  # ---------------------------------------------------------------
  # helper: run many commands in one SLURM job (or locally)
  # ---------------------------------------------------------------
  def run_multi(tag, sbatch_dir, cmd_list)
    if options[:slurm]
      script = write_sbatch_script(tag, sbatch_dir, cmd_list.join("\n\n"))
      out    = `sbatch #{script}`
      if out =~ /Submitted batch job (\d+)/
        @job_ids << $1
        puts "[SLURM_JOBS] #{@job_ids.join(',')}"
      else
        warn "[#{module_key}] sbatch failed for #{tag}: #{out.strip}"
      end
    else
      cmd_list.each do |cmd|
        puts "[#{module_key}][#{tag}] RUN: #{cmd}"
        system(cmd) or warn "[#{module_key}] ERROR on #{tag}"
      end
    end
  end
end

AsymmetryInjectionPWRunner.run!
