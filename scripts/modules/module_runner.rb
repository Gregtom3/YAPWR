require 'yaml'
require 'fileutils'
require 'optparse'

class ModuleRunner
  attr_reader :options, :project, :user_configs, :out_root

  def self.run!
    new.run
  end

  def initialize
    @options = { slurm: false, deps: nil }
    @project, @user_configs = parse_cli(@options)
    @out_root = File.join('out', @project)
    abort "ERROR: '#{@out_root}' does not exist" unless Dir.exist?(@out_root)
    @job_ids = []
  end

  def run
    Dir.glob(File.join(@out_root, 'config_*', '**', 'tree_info.yaml')).sort.each do |info_path|
      leaf_dir = File.dirname(info_path)
      tag      = File.basename(leaf_dir)
      next unless keep_leaf?(tag, info_path)

      info           = YAML.load_file(info_path)
      orig_tfile     = info.fetch('tfile')
      tree_name      = info.fetch('ttree')
      filtered_tfile = File.join(leaf_dir, File.basename(orig_tfile))

      if needs_filtered_file? && !File.exist?(filtered_tfile)
        warn "[#{module_key}][#{tag}] WARNING: filtered file not found: #{filtered_tfile}"
        next
      end
  
      cfg_dir  = ascend_to_config_dir(leaf_dir)
      cfg_name = File.basename(cfg_dir).sub(/^config_/, '')
      next if user_configs.any? && user_configs.none? { |c| File.basename(cfg_dir).include?(c) }

      primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")

      context = {
        info_path:      info_path,
        leaf_dir:       leaf_dir,
        tag:            tag,
        cfg_dir:        cfg_dir,
        cfg_name:       cfg_name,
        primary_yaml:   primary_yaml,
        filtered_tfile: filtered_tfile,
        orig_tfile:     orig_tfile,
        tree_name:      tree_name
      }

      process_leaf(context)
    end

    puts "[SLURM_JOBS] #{@job_ids.join(',')}" if options[:slurm] && @job_ids.any?
  end

  # ------------------------------------------------------------------
  # Default per-leaf behavior (subclasses can override entirely)
  # ------------------------------------------------------------------
  def process_leaf(ctx)
    primary_yaml = ctx[:primary_yaml]
    unless File.exist?(primary_yaml)
      warn "[#{module_key}][#{ctx[:tag]}] WARNING: primary YAML not found: #{primary_yaml}"
      return
    end

    outdir   = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)
    out_yaml = File.join(outdir, result_yaml_name(ctx[:tag]))

    cmd = build_root_cmd(ctx.merge(outdir: outdir, out_yaml: out_yaml))
    run_job(ctx[:tag], outdir, cmd)
  end

  # ------------------------------------------------------------------
  # Hooks to override
  # ------------------------------------------------------------------
  def needs_filtered_file?
    true
  end
  
  def module_key
    raise NotImplementedError
  end

  def out_subdir
    "module-out___#{module_key}"
  end

  def result_yaml_name(_tag)
    "#{module_key}.yaml"
  end

  def keep_leaf?(_tag, _info_path)
    true
  end

  # ctx will include everything in process_leaf
  def macro_call(ctx)
    # default assumes 4-arg macro
    %Q{'src/modules/#{module_key}.C("#{ctx[:orig_tfile]}","#{ctx[:tree_name]}","#{ctx[:primary_yaml]}","#{ctx[:out_yaml]}")'}
  end

  def slurm_job_name(tag)
    "#{module_key}_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '1000', cpus: 1 }
  end

  # ------------------------------------------------------------------
  # Internal (don’t override unless you know why)
  # ------------------------------------------------------------------
  private

  def parse_cli(opts_hash)
      opts = OptionParser.new do |o|
        o.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] [--maxEntries N] PROJECT_NAME [CONFIG...]"
    
        o.on('--slurm', 'Submit via sbatch') do
          opts_hash[:slurm] = true
        end
    
        o.on('--dependency D', 'Pass "afterok:IDs" to sbatch --dependency') do |d|
          opts_hash[:deps] = d
        end
    
        o.on('--maxEntries N', '--maxEntries=N', Integer, 'Limit max entries (positive)') do |n|
          opts_hash[:max_entries] = n if n.positive?
        end
    
        o.on('-h', '--help', 'Show this help') do
          puts o
          exit
        end
      end
    
      # use parse! so options are extracted from anywhere in ARGV
      opts.parse!(ARGV)
    
      project   = ARGV.shift or abort opts.banner
      user_cfgs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }
    
      # default to -1 if not set
      opts_hash[:max_entries] ||= -1
    
      [project, user_cfgs]
  end


  def ascend_to_config_dir(dir)
    d = dir
    d = File.dirname(d) until File.basename(d).start_with?('config_') || d == '/' || d == '.'
    d
  end

  def build_root_cmd(ctx)
    ['root', '-l', '-b', '-q', macro_call(ctx)].join(' ')
  end

  def run_job(tag, outdir, cmd)
    if options[:slurm]
      script = write_sbatch_script(tag, outdir, cmd)
      out = `sbatch #{script}`
      if out =~ /Submitted batch job (\d+)/
        @job_ids << $1
        puts "[SLURM_JOBS] #{@job_ids.join(',')}"
      else
        warn "[#{module_key}] sbatch failed for #{tag}: #{out.strip}"
      end
    else
      puts "[#{module_key}][#{tag}] RUN: #{cmd}"
      system(cmd) or warn "[#{module_key}] ERROR running for #{tag}"
    end
  end

  def write_sbatch_script(tag, outdir, cmd)
    s = +"#!/bin/bash\n"
    s << "#SBATCH --job-name=#{slurm_job_name(tag)}\n"
    s << "#SBATCH --output=#{outdir}/#{module_key}_#{tag}.out\n"
    s << "#SBATCH --error=#{outdir}/#{module_key}_#{tag}.err\n"
    s << "#SBATCH --time=#{slurm_directives[:time]}\n"
    s << "#SBATCH --mem-per-cpu=#{slurm_directives[:mem_per_cpu]}\n"
    s << "#SBATCH --cpus-per-task=#{slurm_directives[:cpus]}\n"
    s << "#SBATCH --dependency=#{options[:deps]}\n" if options[:deps]
    s << "cd #{Dir.pwd}\n"
    s << "#{cmd}\n"

    script_path = File.join(outdir, "run_#{module_key}_#{tag}.slurm")
    File.write(script_path, s)
    FileUtils.chmod('+x', script_path)
    script_path
  end
end
