#!/bin/sh
set -eu

EXIT_INCONCLUSIVE=20
EXIT_USAGE=64

APPROVED_REPRODUCER_SHA256=bdf2948b6329b302130f2912983e8054b7e1ae34af0b53756b6fd4968bfb985a
APPROVED_B7_PATCH_SHA256=418b0c8870544f4de777b10ec7938d80437789dc9beb3ee8dadc42065b5a32a5
KNOWN_B7_BASELINE_LIBRARY_SHA256=262fb1e449da668a6a97dd88be69529ce178fd5bd27914ba32eb16155c03e409
KNOWN_B7_PATCHED_LIBRARY_SHA256=8edfdb04646bfb8bbe9e0efddb02a375b988b6566500931a5c40f8522a9ed423

usage_error()
{
    echo "USAGE: $*" >&2
    exit "$EXIT_USAGE"
}

inconclusive()
{
    echo "INCONCLUSIVE: $*" >&2
    exit "$EXIT_INCONCLUSIVE"
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 ||
        inconclusive "required command is unavailable: $1"
}

file_sha256()
{
    sha256sum "$1" | awk '{print $1}'
}

value_sha256()
{
    printf '%s' "$1" | sha256sum | awk '{print $1}'
}

canonical_file()
{
    readlink -f "$1"
}

canonical_directory()
{
    (CDPATH= cd -- "$1" && pwd -P)
}

count_status_column()
{
    column=$1
    awk -v column="$column" '
        substr($0, 1, 2) != "??" &&
        substr($0, column, 1) != " " { count++ }
        END { print count + 0 }
    ' "$2"
}

[ -n "${RUSTICL_LIBRARY-}" ] ||
    usage_error "set RUSTICL_LIBRARY to the exact candidate library file"
[ -n "${RUSTICL_ICD_FILE-}" ] ||
    usage_error "set RUSTICL_ICD_FILE to the exact Rusticl .icd file"
[ -n "${EXPECTED_RUSTICL_SHA256-}" ] ||
    usage_error "set EXPECTED_RUSTICL_SHA256 to the candidate-library SHA-256"
[ -n "${MESA_SOURCE-}" ] ||
    usage_error "set MESA_SOURCE to the exact Mesa checkout or linked worktree"
[ -n "${EXPECTED_MESA_COMMIT-}" ] ||
    usage_error "set EXPECTED_MESA_COMMIT to the exact 40-hex commit"
[ -n "${B7_STATE-}" ] ||
    usage_error "set B7_STATE to absent or present as the expected state"
[ -n "${RUNTIME_LABEL-}" ] ||
    usage_error "set a unique non-secret RUNTIME_LABEL"
[ -n "${PROVENANCE_OUT-}" ] ||
    usage_error "set PROVENANCE_OUT to a new manifest path outside Mesa"

for required_command in awk basename chmod date dirname git grep mktemp mv \
    readlink rm sed sha256sum tr uname wc; do
    require_command "$required_command"
done

case "$B7_STATE" in
    absent|present) ;;
    *) usage_error "B7_STATE must be absent or present" ;;
esac

printf '%s\n' "$RUNTIME_LABEL" |
    grep -Eq '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' ||
    usage_error "RUNTIME_LABEL must use 1-128 ASCII letters, digits, dot, underscore, or hyphen"

printf '%s\n' "$EXPECTED_MESA_COMMIT" |
    grep -Eq '^[0-9a-f]{40}$' ||
    usage_error "EXPECTED_MESA_COMMIT must be an exact lowercase 40-hex commit"
printf '%s\n' "$EXPECTED_RUSTICL_SHA256" |
    grep -Eq '^[0-9a-f]{64}$' ||
    usage_error "EXPECTED_RUSTICL_SHA256 must be an exact lowercase SHA-256"

if printf '%s' "${RUSTICL_DEBUG-}" |
    tr ', ' '\n\n' |
    grep -qx 'sync'; then
    inconclusive "RUSTICL_DEBUG contains sync and masks the ordering oracle"
fi

script_directory=$(canonical_directory "$(dirname -- "$0")") ||
    inconclusive "cannot resolve the gate directory"
source_path="$script_directory/fup_b4_2a_cl_mem_lifetime.c"
if [ ! -f "$source_path" ]; then
    inconclusive "reproducer source is missing: $source_path"
fi
source_sha256=$(file_sha256 "$source_path")
if [ "$source_sha256" != "$APPROVED_REPRODUCER_SHA256" ]; then
    inconclusive "reproducer SHA-256 is not the approved T1 value (actual=$source_sha256)"
fi

if [ ! -f "$RUSTICL_LIBRARY" ]; then
    inconclusive "Rusticl candidate is not a regular file: $RUSTICL_LIBRARY"
fi
if [ ! -f "$RUSTICL_ICD_FILE" ]; then
    inconclusive "Rusticl ICD configuration is not a regular file: $RUSTICL_ICD_FILE"
fi
if [ -e "$PROVENANCE_OUT" ]; then
    inconclusive "refusing to overwrite existing manifest: $PROVENANCE_OUT"
fi
provenance_parent=$(dirname -- "$PROVENANCE_OUT")
if [ ! -d "$provenance_parent" ]; then
    inconclusive "manifest parent directory does not exist: $provenance_parent"
fi
if [ -n "${FUP_BINARY-}" ] && [ ! -f "$FUP_BINARY" ]; then
    inconclusive "FUP_BINARY is not a regular file: $FUP_BINARY"
fi

candidate_library=$(canonical_file "$RUSTICL_LIBRARY") ||
    inconclusive "cannot resolve Rusticl candidate: $RUSTICL_LIBRARY"
candidate_library_sha256=$(file_sha256 "$candidate_library")
if [ "$candidate_library_sha256" != "$EXPECTED_RUSTICL_SHA256" ]; then
    inconclusive "Rusticl candidate SHA-256 differs from expectation (actual=$candidate_library_sha256)"
fi

case "$candidate_library_sha256" in
    "$KNOWN_B7_BASELINE_LIBRARY_SHA256")
        b7_state_from_known_library_hash=absent
        ;;
    "$KNOWN_B7_PATCHED_LIBRARY_SHA256")
        b7_state_from_known_library_hash=present
        ;;
    *)
        b7_state_from_known_library_hash=unknown
        ;;
esac
scratch_directory=$(mktemp -d "${TMPDIR:-/tmp}/fup-b4-2a-gate.XXXXXX") ||
    inconclusive "cannot create temporary analysis directory"
manifest_tmp=
cleanup()
{
    if [ -n "${scratch_directory-}" ] &&
       [ -d "$scratch_directory" ]; then
        rm -rf -- "$scratch_directory"
    fi
    if [ -n "${manifest_tmp-}" ] &&
       [ -f "$manifest_tmp" ]; then
        rm -f -- "$manifest_tmp"
    fi
}
trap cleanup EXIT HUP INT TERM

icd_entries="$scratch_directory/icd-entries.txt"
awk '
    {
        sub(/\r$/, "")
        sub(/^[[:space:]]+/, "")
        sub(/[[:space:]]+$/, "")
        if ($0 != "" && substr($0, 1, 1) != "#")
            print
    }
' "$RUSTICL_ICD_FILE" >"$icd_entries"
icd_entry_count=$(wc -l <"$icd_entries" | awk '{print $1}')
if [ "$icd_entry_count" -ne 1 ]; then
    inconclusive "Rusticl ICD file must contain exactly one active library entry"
fi
icd_library_entry=$(sed -n '1p' "$icd_entries")
icd_file=$(canonical_file "$RUSTICL_ICD_FILE") ||
    inconclusive "cannot resolve Rusticl ICD file"
icd_file_sha256=$(file_sha256 "$icd_file")

case "$icd_library_entry" in
    /*)
        if [ ! -f "$icd_library_entry" ]; then
            inconclusive "absolute ICD library entry does not name a regular file"
        fi
        icd_entry_resolved=$(canonical_file "$icd_library_entry") ||
            inconclusive "cannot resolve absolute ICD library entry"
        if [ "$icd_entry_resolved" != "$candidate_library" ]; then
            inconclusive "ICD entry resolves to a different library than RUSTICL_LIBRARY"
        fi
        icd_static_resolution=absolute_entry_matches_candidate
        ;;
    */*)
        inconclusive "relative ICD entry contains a directory and cannot be resolved reliably"
        ;;
    *)
        if [ "$icd_library_entry" != "$(basename -- "$RUSTICL_LIBRARY")" ] &&
           [ "$icd_library_entry" != "$(basename -- "$candidate_library")" ]; then
            inconclusive "ICD basename does not match the selected Rusticl candidate"
        fi
        icd_entry_resolved=UNRESOLVED_DYNAMIC_LOADER_SEARCH
        icd_static_resolution=basename_matches_candidate_but_search_path_is_ambiguous
        ;;
esac

if [ -n "${OCL_ICD_VENDORS-}" ]; then
    ocl_icd_vendors_set=yes
    ocl_icd_vendors_sha256=$(value_sha256 "$OCL_ICD_VENDORS")
    if [ -f "$OCL_ICD_VENDORS" ]; then
        configured_icd=$(canonical_file "$OCL_ICD_VENDORS") ||
            inconclusive "cannot resolve OCL_ICD_VENDORS file"
        if [ "$configured_icd" != "$icd_file" ]; then
            inconclusive "OCL_ICD_VENDORS selects a different ICD file"
        fi
        ocl_icd_vendors_relation=exact_icd_file
    elif [ -d "$OCL_ICD_VENDORS" ]; then
        configured_icd_directory=$(canonical_directory "$OCL_ICD_VENDORS") ||
            inconclusive "cannot resolve OCL_ICD_VENDORS directory"
        icd_parent=$(canonical_directory "$(dirname -- "$icd_file")") ||
            inconclusive "cannot resolve ICD parent directory"
        if [ "$configured_icd_directory" != "$icd_parent" ]; then
            inconclusive "RUSTICL_ICD_FILE is outside OCL_ICD_VENDORS"
        fi
        ocl_icd_vendors_relation=directory_contains_icd
    else
        inconclusive "OCL_ICD_VENDORS is set but is neither a file nor directory"
    fi
else
    ocl_icd_vendors_set=no
    ocl_icd_vendors_sha256=NOT_SET
    ocl_icd_vendors_relation=loader_default_search_not_proven
fi

if [ -n "${LD_LIBRARY_PATH-}" ]; then
    ld_library_path_set=yes
    ld_library_path_sha256=$(value_sha256 "$LD_LIBRARY_PATH")
else
    ld_library_path_set=no
    ld_library_path_sha256=NOT_SET
fi
if [ -n "${LD_PRELOAD-}" ]; then
    ld_preload_set=yes
    ld_preload_sha256=$(value_sha256 "$LD_PRELOAD")
else
    ld_preload_set=no
    ld_preload_sha256=NOT_SET
fi

if ! git -C "$MESA_SOURCE" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    inconclusive "MESA_SOURCE is not a Git working tree or linked worktree"
fi
mesa_toplevel=$(git -C "$MESA_SOURCE" rev-parse --show-toplevel) ||
    inconclusive "cannot resolve Mesa worktree root"
mesa_toplevel=$(canonical_directory "$mesa_toplevel") ||
    inconclusive "cannot canonicalize Mesa worktree root"
mesa_git_dir=$(git -C "$mesa_toplevel" rev-parse --absolute-git-dir) ||
    inconclusive "cannot resolve Mesa worktree Git directory"
mesa_git_common_dir_raw=$(git -C "$mesa_toplevel" rev-parse --git-common-dir) ||
    inconclusive "cannot resolve Mesa common Git directory"
case "$mesa_git_common_dir_raw" in
    /*) mesa_git_common_dir=$(canonical_directory "$mesa_git_common_dir_raw") ;;
    *) mesa_git_common_dir=$(canonical_directory "$mesa_toplevel/$mesa_git_common_dir_raw") ;;
esac

manifest_parent=$(canonical_directory "$provenance_parent") ||
    inconclusive "cannot canonicalize manifest parent"
case "$manifest_parent/" in
    "$mesa_toplevel/"*)
        inconclusive "PROVENANCE_OUT must be outside the Mesa worktree"
        ;;
esac

actual_mesa_commit=$(git -C "$mesa_toplevel" rev-parse HEAD) ||
    inconclusive "cannot resolve Mesa HEAD"
if [ "$actual_mesa_commit" != "$EXPECTED_MESA_COMMIT" ]; then
    inconclusive "Mesa HEAD differs from the exact expected commit (actual=$actual_mesa_commit)"
fi
mesa_branch=$(git -C "$mesa_toplevel" symbolic-ref --quiet --short HEAD 2>/dev/null ||
    printf '%s' DETACHED)

mesa_status="$scratch_directory/mesa-status.txt"
mesa_staged_diff="$scratch_directory/mesa-staged.patch"
mesa_unstaged_diff="$scratch_directory/mesa-unstaged.patch"
mesa_total_diff="$scratch_directory/mesa-total.patch"
git -C "$mesa_toplevel" status --porcelain=v1 --untracked-files=all >"$mesa_status" ||
    inconclusive "cannot inspect Mesa status"
git -C "$mesa_toplevel" diff --cached --binary --no-ext-diff >"$mesa_staged_diff" ||
    inconclusive "cannot inspect staged Mesa changes"
git -C "$mesa_toplevel" diff --binary --no-ext-diff >"$mesa_unstaged_diff" ||
    inconclusive "cannot inspect unstaged Mesa changes"
git -C "$mesa_toplevel" diff HEAD --binary --no-ext-diff >"$mesa_total_diff" ||
    inconclusive "cannot inspect total Mesa changes"

mesa_staged_count=$(count_status_column 1 "$mesa_status")
mesa_unstaged_count=$(count_status_column 2 "$mesa_status")
mesa_untracked_count=$(awk '
    substr($0, 1, 2) == "??" { count++ }
    END { print count + 0 }
' "$mesa_status")
mesa_status_sha256=$(file_sha256 "$mesa_status")
mesa_staged_diff_sha256=$(file_sha256 "$mesa_staged_diff")
mesa_unstaged_diff_sha256=$(file_sha256 "$mesa_unstaged_diff")
mesa_total_diff_sha256=$(file_sha256 "$mesa_total_diff")

default_b7_patch="$script_directory/../../patches/b7/rusticl_clSetKernelArg_context_validation.patch"
b7_patch_file=${B7_PATCH_FILE:-$default_b7_patch}
if [ ! -f "$b7_patch_file" ]; then
    inconclusive "known B7 patch file is missing: $b7_patch_file"
fi
b7_patch_file=$(canonical_file "$b7_patch_file") ||
    inconclusive "cannot resolve B7 patch file"
b7_patch_sha256=$(file_sha256 "$b7_patch_file")
if [ "$b7_patch_sha256" != "$APPROVED_B7_PATCH_SHA256" ]; then
    inconclusive "B7 patch evidence does not match its approved SHA-256"
fi

if git -C "$mesa_toplevel" apply --check "$b7_patch_file" >/dev/null 2>&1; then
    b7_forward_applies=yes
else
    b7_forward_applies=no
fi
if git -C "$mesa_toplevel" apply --reverse --check "$b7_patch_file" >/dev/null 2>&1; then
    b7_reverse_applies=yes
else
    b7_reverse_applies=no
fi
case "$b7_forward_applies:$b7_reverse_applies" in
    yes:no) detected_b7_state=absent ;;
    no:yes) detected_b7_state=present ;;
    *) detected_b7_state=unknown ;;
esac
if [ "$detected_b7_state" = unknown ]; then
    b7_provenance_result=INCONCLUSIVE
    b7_provenance_issue=forward_reverse_apply_checks_are_ambiguous
elif [ "$detected_b7_state" != "$B7_STATE" ]; then
    b7_provenance_result=INCONCLUSIVE
    b7_provenance_issue=expected_state_disagrees_with_source_derived_state
else
    b7_provenance_result=PASS
    b7_provenance_issue=none
fi
if [ "$b7_state_from_known_library_hash" != unknown ] &&
   [ "$b7_state_from_known_library_hash" != "$detected_b7_state" ]; then
    b7_provenance_result=INCONCLUSIVE
    b7_provenance_issue=known_library_hash_disagrees_with_source_derived_state
fi

kernel_rs="$mesa_toplevel/src/gallium/frontends/rusticl/api/kernel.rs"
if [ ! -f "$kernel_rs" ]; then
    inconclusive "Rusticl api/kernel.rs is missing"
fi
kernel_rs_sha256=$(file_sha256 "$kernel_rs")
b7_guard_count=$(grep -c 'context != k\.prog\.context' "$kernel_rs" || true)

git_provenance_result=PASS
git_provenance_issue=none
if [ "$mesa_untracked_count" -ne 0 ]; then
    mesa_local_change_classification=unexplained_untracked_files
    git_provenance_result=INCONCLUSIVE
    git_provenance_issue=untracked_files_are_not_approved
elif [ ! -s "$mesa_status" ]; then
    mesa_local_change_classification=clean
elif [ "$detected_b7_state" = present ] &&
     [ "$mesa_total_diff_sha256" = "$APPROVED_B7_PATCH_SHA256" ]; then
    mesa_local_change_classification=exact_approved_b7_patch
else
    mesa_local_change_classification=unexplained_local_changes
    git_provenance_result=INCONCLUSIVE
    git_provenance_issue=local_changes_are_not_exactly_the_approved_b7_patch
fi

runtime_loaded_library_verification=PENDING_OPENCL_PROCESS_EVIDENCE
runtime_provenance_result=INCONCLUSIVE
if [ "$git_provenance_result" = PASS ] &&
   [ "$b7_provenance_result" = PASS ]; then
    gate_result=INCONCLUSIVE_RUNTIME_BINDING_PENDING
else
    gate_result=INCONCLUSIVE_GIT_OR_B7_PROVENANCE
fi

manifest_tmp=$(mktemp "$provenance_parent/.fup-b4-2a-manifest.XXXXXX") ||
    inconclusive "cannot create temporary manifest"
{
    echo "gate_version=FUP-B4-2A-T1.1"
    echo "gate_result=$gate_result"
    echo "timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "runtime_label=$RUNTIME_LABEL"
    echo "runtime_provenance_result=$runtime_provenance_result"
    echo "runtime_loaded_library_verification=$runtime_loaded_library_verification"
    echo "rusticl_candidate_library=$candidate_library"
    echo "rusticl_candidate_library_sha256=$candidate_library_sha256"
    echo "rusticl_icd_file=$icd_file"
    echo "rusticl_icd_file_sha256=$icd_file_sha256"
    echo "rusticl_icd_library_entry=$icd_library_entry"
    echo "rusticl_icd_entry_resolved=$icd_entry_resolved"
    echo "rusticl_icd_static_resolution=$icd_static_resolution"
    echo "ocl_icd_vendors_set=$ocl_icd_vendors_set"
    echo "ocl_icd_vendors_sha256=$ocl_icd_vendors_sha256"
    echo "ocl_icd_vendors_relation=$ocl_icd_vendors_relation"
    echo "ld_library_path_set=$ld_library_path_set"
    echo "ld_library_path_sha256=$ld_library_path_sha256"
    echo "ld_preload_set=$ld_preload_set"
    echo "ld_preload_sha256=$ld_preload_sha256"
    echo "mesa_worktree=$mesa_toplevel"
    echo "mesa_git_dir=$mesa_git_dir"
    echo "mesa_git_common_dir=$mesa_git_common_dir"
    echo "mesa_head=$actual_mesa_commit"
    echo "mesa_branch=$mesa_branch"
    echo "mesa_staged_count=$mesa_staged_count"
    echo "mesa_unstaged_count=$mesa_unstaged_count"
    echo "mesa_untracked_count=$mesa_untracked_count"
    echo "mesa_status_porcelain_v1_sha256=$mesa_status_sha256"
    echo "mesa_staged_diff_sha256=$mesa_staged_diff_sha256"
    echo "mesa_unstaged_diff_sha256=$mesa_unstaged_diff_sha256"
    echo "mesa_total_diff_sha256=$mesa_total_diff_sha256"
    echo "mesa_local_change_classification=$mesa_local_change_classification"
    echo "git_provenance_result=$git_provenance_result"
    echo "git_provenance_issue=$git_provenance_issue"
    echo "mesa_status_porcelain_v1_begin"
    sed 's/^/mesa_status=/' "$mesa_status"
    echo "mesa_status_porcelain_v1_end"
    echo "b7_expected_state=$B7_STATE"
    echo "b7_detected_state=$detected_b7_state"
    echo "b7_provenance_result=$b7_provenance_result"
    echo "b7_provenance_issue=$b7_provenance_issue"
    echo "b7_state_from_known_library_hash=$b7_state_from_known_library_hash"
    echo "b7_patch_file=$b7_patch_file"
    echo "b7_patch_sha256=$b7_patch_sha256"
    echo "b7_forward_applies=$b7_forward_applies"
    echo "b7_reverse_applies=$b7_reverse_applies"
    echo "b7_guard_count=$b7_guard_count"
    echo "rusticl_api_kernel_rs_sha256=$kernel_rs_sha256"
    echo "reproducer_source=$source_path"
    echo "reproducer_approved_sha256=$APPROVED_REPRODUCER_SHA256"
    echo "reproducer_actual_sha256=$source_sha256"
    echo "rusticl_debug_sync=0"
    echo "system=$(uname -srm)"
    if [ -n "${FUP_BINARY-}" ]; then
        echo "reproducer_binary=$(canonical_file "$FUP_BINARY")"
        echo "reproducer_binary_sha256=$(file_sha256 "$FUP_BINARY")"
    else
        echo "reproducer_binary=NOT_BUILT"
        echo "reproducer_binary_sha256=NOT_AVAILABLE"
    fi
} >"$manifest_tmp"
chmod 600 "$manifest_tmp"
mv "$manifest_tmp" "$PROVENANCE_OUT"
manifest_tmp=

echo "STATIC CHECKS PASS"
echo "manifest=$PROVENANCE_OUT"
echo "reproducer_sha256=$source_sha256"
echo "rusticl_candidate_sha256=$candidate_library_sha256"
if [ "$git_provenance_result" != PASS ] ||
   [ "$b7_provenance_result" != PASS ]; then
    echo "INCONCLUSIVE: Git or B7 provenance contains unexplained deviations; see manifest" >&2
    exit "$EXIT_INCONCLUSIVE"
fi
echo "INCONCLUSIVE: actual loaded Rusticl library remains unverified; OpenCL process evidence is required" >&2
exit "$EXIT_INCONCLUSIVE"
