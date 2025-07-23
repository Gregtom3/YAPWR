#!/usr/bin/env ruby
# coding: utf-8
require 'optparse'
require 'fileutils'
require 'yaml'
require 'shellwords'
require 'open3'
require 'set'
# ------------------------------------------------------------------
#  User-constants
# ------------------------------------------------------------------
VOLATILE_PROJECT   = 'pipi0-paper-v5-pass2'
VOLATILE_SUFFIX    = '*merged_cuts_noPmin*'
VOLATILE_TREE_NAME = 'dihadron_cuts_noPmin'

# ------------------------------------------------------------------
#  CLI parsing
# ------------------------------------------------------------------
options = { append: false, maxEntries: nil, maxFiles: nil, slurm: false, is_running_on_slurm: false,   doAll: false }
optlist  = []                                     # remember original flags

parser = OptionParser.new do |opts|
  opts.banner = <<~TXT
     Usage: #{$0} [options]  PROJECT  RUNCARD  CONFIG1 [CONFIG2 ...]
       --append           do not overwrite existing out/<PROJECT>; skip filterTree
       --maxEntries N     pass N to filterTree as entry limit
       --maxFiles   M     only create M tree_info.yaml per config
       --slurm            submit one Slurm job per CONFIG instead of running now
  TXT

  opts.on('--append')               { options[:append] = true ; optlist << '--append' }
  opts.on('--maxEntries N', Integer){ |n| options[:maxEntries] = n ; optlist += ['--maxEntries', n.to_s] }
  opts.on('--maxFiles M',  Integer){ |m| options[:maxFiles]   = m ; optlist += ['--maxFiles',   m.to_s] }
  opts.on('--slurm')                { options[:slurm]  = true }
  opts.on('--is_running_on_slurm')                { options[:is_running_on_slurm]  = true }
  opts.on('--doAll', 'Ignore the tag‐filters and use every merged file') do
    options[:doAll] = true
    optlist << '--doAll'
  end
  opts.on('-h','--help'){ puts opts ; exit }
end

parser.parse!

FILTER_TAGS = {
  'piminus_pi0'     => %w[Fall2018_RGA_outbending MC_RGA_outbending],
  'piminus_piminus' => %w[Fall2018_RGA_outbending MC_RGA_outbending],
  'piplus_pi0'      => %w[Fall2018Spring2019_RGA_inbending MC_RGA_inbending],
  'piplus_piplus'   => %w[Fall2018Spring2019_RGA_inbending MC_RGA_inbending],
  'piplus_piminus'  => %w[Fall2018Spring2019_RGA_inbending MC_RGA_inbending],
  'pi0_pi0'         => %w[Fall2018Spring2019_RGA_inbending MC_RGA_inbending]
}.freeze

# ------------------------------------------------------------------
#  Positional arguments
# ------------------------------------------------------------------
if ARGV.size < 3
  warn parser.banner
  exit 1
end

project_name = ARGV.shift
runcard_path = ARGV.shift
config_files = ARGV

abort "runcard '#{runcard_path}' not found" unless File.file?(runcard_path)

# ------------------------------------------------------------------
#  If --slurm → emit one batch file per config and exit
# ------------------------------------------------------------------
if options[:slurm]
  project_root = File.join('out', project_name)
  FileUtils.mkdir_p(project_root)

  config_files.each do |cfg_path|
    cfg_name  = File.basename(cfg_path, File.extname(cfg_path))
    cfg_dir   = File.join(project_root, "config_#{cfg_name}")
    FileUtils.mkdir_p(cfg_dir)

    # rebuild CLI: same flags except --slurm, and *only* this config
    cli = ['ruby',
           File.realpath(__FILE__),
           *optlist,
           project_name,
           "--is_running_on_slurm",
           File.realpath(runcard_path),
           File.realpath(cfg_path)
          ].shelljoin

    batch = <<~SLURM
      #!/bin/bash
      #SBATCH --job-name=#{project_name}_#{cfg_name}
      #SBATCH --output=#{project_root}/pipeline_#{cfg_name}.out
      #SBATCH --error=#{project_root}/pipeline_#{cfg_name}.err
      #SBATCH --time=24:00:00
      #SBATCH --mem-per-cpu=4000
      #SBATCH --cpus-per-task=4

      cd #{Dir.pwd}
      echo "== #{project_name}/#{cfg_name} started $(date)"
      #{cli}
      echo "== finished $(date)"
    SLURM

    slurm_script = File.join(cfg_dir, 'run_pipeline.slurm')
    File.write(slurm_script, batch)
    FileUtils.chmod('+x', slurm_script)

    puts "[run_project] sbatch #{slurm_script}"
    system('sbatch', slurm_script) or abort "sbatch failed for #{cfg_name}"
  end
  exit 0
end
# ------------------------------------------------------------------
#  Immediate (non-Slurm) run
# ------------------------------------------------------------------

# ---------- runcard ------------------------------------------------
runcard = YAML.load_file(runcard_path)
modules = runcard['modules']
abort "runcard needs an array 'modules'" unless modules.is_a?(Array)

# ---------- prepare out/ tree -------------------------------------
out_root = File.join('out', project_name)

# ---------- locate volatile ROOT files -----------------------------
base_dir = File.join('/volatile/clas12/users/gmat/clas12analysis.sidis.data',
                     'clas12_dihadrons','projects',VOLATILE_PROJECT,'data')
merged_files = Dir.glob(File.join(base_dir,'*',VOLATILE_SUFFIX))
abort "No merged_cuts files under #{base_dir}" if merged_files.empty?

# ---------- create config-specific dirs and tree_info.yaml ---------
config_files.each do |cfg|
  next unless File.file?(cfg)

  cfg_name = File.basename(cfg, File.extname(cfg))
  cfg_dir  = File.join(out_root, "config_#{cfg_name}")

  unless options[:append]
      if Dir.exist?(cfg_dir)
        #––– interactive only when we have a TTY –––
        answer =
          if $stdin.tty?            # local run: ask user
            print "Directory '#{cfg_dir}' exists. Overwrite? [y/N]: "
            STDIN.gets&.chomp&.downcase
          else                      # Slurm: no TTY → auto-yes
            'yes'
          end
    
        exit 0 unless %w[y yes].include?(answer)
        FileUtils.rm_rf(cfg_dir)
      end
  end
  FileUtils.mkdir_p(cfg_dir)
  FileUtils.cp(cfg, cfg_dir) unless options[:append]

  unless options[:append]
    File.open(File.join(cfg_dir, File.basename(cfg)), 'a+') do |file|
      file.write "\n"
      file.write 'num_entries: ' + String(options[:maxEntries].nil? ? -1 : options[:maxEntries])
    end
  end
    
  unless options[:append]
    File.write(File.join(cfg_dir, 'volatile_project.txt'), VOLATILE_PROJECT)

    usable = if options[:doAll]
               merged_files
             else
               merged_files.select do |fp|
                 pair = File.basename(File.dirname(fp))
                 tag  = File.basename(fp).split('_merged_cuts_noPmin').first
                 FILTER_TAGS.fetch(pair, []).include?(tag)
               end
             end
    usable = usable.reject { |f| File.basename(File.dirname(f)) == 'pi0_pi0' }
    usable = usable.first(options[:maxFiles]) if options[:maxFiles]&.positive?

      
    # usable = merged_files.reject do |f|
    #   File.basename(File.dirname(f)) != 'piplus_piminus'
    # end

      
    usable.each do |fp|
      pair = File.basename(File.dirname(fp))
      tag  = File.basename(fp).split('_merged_cuts_noPmin').first
      leaf = File.join(cfg_dir, pair, tag)
      FileUtils.mkdir_p(leaf)
      info = { 'tfile'=>File.absolute_path(fp), 'ttree'=>VOLATILE_TREE_NAME }
      File.write(File.join(leaf,'tree_info.yaml'), info.to_yaml)
    end

    puts "Prepared #{cfg_dir} (#{usable.size} trees)"
  end
end

puts "\nDirectory tree:"
system('tree', out_root)

# ---------- helper to launch each module ---------------------------
def invoke(name, *args)
  puts "\n=> #{name}: #{args.shelljoin}"
  system(*args) or abort "#{name} failed"
end

# ---------- wait until a list of Slurm jobs is done ----------------
def wait_for_slurm_jobs(ids, poll = 60)
  return if ids.empty?

  # normalize to integers and store in a Set for fast lookup
  target = ids.map(&:to_i).to_set
  puts "[run_project] Waiting for filterTree jobs #{target.to_a.join(',')} …"

  loop do
    # ask Slurm for jobs
    out, status = Open3.capture2("squeue -u #{ENV['USER']} -h -o '%A'")
    unless status.success?
      warn "[run_project] warning: squeue failed (exit #{status.exitstatus})"
      sleep poll
      next
    end

    # figure out which of your target IDs are still running
    running = out.split.map(&:to_i).to_set
    break if (target & running).empty?

    sleep poll
  end

  puts "[run_project] ...all filterTree jobs finished"
end

purity_ids = []


modules.each do |mod|
  case mod
  when 'filterTree'
      # if options[:is_running_on_slurm]
      #   # run in Slurm‑fan‑out mode and capture job‑IDs
      #   cmd = ['ruby','./scripts/modules/module___filterTree.rb',
      #          '--slurm', project_name]
      #   cmd << options[:maxEntries].to_s if options[:maxEntries]
      #   out = `#{cmd.shelljoin}`
      #   puts out
    
      #   filter_ids = []
      #   out.each_line.grep(/\[SLURM_JOBS\]/) { |ln| filter_ids += ln.split.last.split(/,/) }
    
      #   # BLOCK here until every filterTree job is done
      #   wait_for_slurm_jobs(filter_ids)
      # else
      #   # immediate, non‑Slurm execution
      #   args = ['ruby','./scripts/modules/module___filterTree.rb', project_name]
      #   args << options[:maxEntries].to_s if options[:maxEntries]
      #   invoke('filterTree', *args)


      # filterTree is pretty 
      args = ['ruby','./scripts/modules/module___filterTree.rb', project_name]
      args << options[:maxEntries].to_s if options[:maxEntries]
      invoke('filterTree', *args)

  when 'purityBinning'
    args = ['ruby', './scripts/modules/module___purityBinning.rb']
    args << '--slurm' if options[:is_running_on_slurm]
    args << project_name

    if options[:is_running_on_slurm]
      out = `#{args.shelljoin}`
      puts out
      out.each_line.grep(/\[SLURM_JOBS\]/) do |ln|
        # collect every ID in the comma‐list
        purity_ids += ln.split.last.split(',')
      end
    else
      invoke('purityBinning', *args)
    end

  when 'asymmetry'
    args = ['ruby', './scripts/modules/module___asymmetry.rb']
    args << '--slurm' if options[:is_running_on_slurm]
    if options[:is_running_on_slurm] && purity_ids.any?
      args << '--dependency' << "afterok:#{purity_ids.join(',')}"
    end
    args << project_name
    invoke('asymmetry', *args)

  when 'asymmetry_sideband'
    args = ['ruby', './scripts/modules/module___asymmetry_sideband.rb']
    args << '--slurm' if options[:is_running_on_slurm]
    if options[:is_running_on_slurm] && purity_ids.any?
      args << '--dependency' << "afterok:#{purity_ids.join(',')}"
    end
    args << project_name
    invoke('asymmetry_sideband', *args)

  when 'kinematicBins'
    args = ['ruby', './scripts/modules/module___kinematicBins.rb']
    # Relatively fast, does not necessarily need to be its own job
    # args << '--slurm' if options[:is_running_on_slurm]
    args << project_name
    invoke('kinematicBins', *args)

  when 'baryonContamination'
    args = ['ruby', './scripts/modules/module___baryonContamination.rb']
    # Very fast, does not need to be its own job
    #args << '--slurm' if options[:is_running_on_slurm]
    args << project_name
    invoke('baryonContamination', *args)

  when 'particleMisidentification'
    args = ['ruby', './scripts/modules/module___particleMisidentification.rb']
    # Very fast, does not need to be its own job
    #args << '--slurm' if options[:is_running_on_slurm]
    args << project_name
    invoke('particleMisidentification', *args)

  when 'binMigration'
    args = ['ruby', './scripts/modules/module___binMigration.rb']
    # Very fast, does not need to be its own job
    #args << '--slurm' if options[:is_running_on_slurm]
    args << project_name
    invoke('binMigration', *args)
  else
    warn "WARNING: unknown module '#{mod}' – skipped"
  end
end