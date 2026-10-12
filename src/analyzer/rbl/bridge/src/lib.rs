use std::ffi::{c_char, CStr, CString};
use std::path::Path;

const HEAD_SECS: f64 = 1800.0;

#[repr(C)]
pub struct RblBeat {
    pub time_ms: u32,
    pub number: u16,
    pub tempo_x100: u16,
}

#[repr(C)]
pub struct RblColumn {
    pub low: u8,
    pub mid: u8,
    pub high: u8,
    pub peak: u8,
}

#[repr(C)]
pub struct RblResult {
    pub bpm: f64,
    pub sample_rate: u32,
    pub duration_secs: f64,
    pub beats: *const RblBeat,
    pub beats_len: usize,
    pub key: *const c_char,
    pub columns: *const RblColumn,
    pub columns_len: usize,
    pub overview: *const RblColumn,
    pub overview_len: usize,
    owned_beats: Vec<RblBeat>,
    owned_key: Option<CString>,
    owned_columns: Vec<RblColumn>,
    owned_overview: Vec<RblColumn>,
}

fn analyze(path: &Path) -> Option<RblResult> {
    let mut stream = rbl_audio::MonoStream::open(path).ok()?;
    let rate = stream.sample_rate();
    if rate == 0 {
        return None;
    }
    let cap = (HEAD_SECS * f64::from(rate)) as usize;
    let mut head = Vec::new();
    let mut all_waveform: Option<rbl_analysis::waveform::Builder> = None;
    let mut total_samples = 0_u64;
    while let Some(chunk) = stream.next_chunk() {
        total_samples += chunk.len() as u64;
        if let Some(builder) = all_waveform.as_mut() {
            builder.push(chunk);
        } else if head.len() + chunk.len() > cap {
            head.extend_from_slice(chunk);
            let mut builder = rbl_analysis::waveform::Builder::new(rate);
            builder.push(&head);
            head.truncate(cap);
            all_waveform = Some(builder);
        } else {
            head.extend_from_slice(chunk);
        }
    }
    if head.is_empty() {
        return None;
    }
    let mut analysis =
        rbl_analysis::analyse_with(&head, rate, rbl_analysis::AnalysisPreset::Rbxport.options());
    let duration_secs = if let Some(builder) = all_waveform {
        let duration = total_samples as f64 / f64::from(rate);
        analysis.tempo.extend_to(duration);
        analysis.waveform = builder.finish();
        duration
    } else {
        head.len() as f64 / f64::from(rate)
    };
    let owned_beats: Vec<_> = analysis
        .tempo
        .beats
        .iter()
        .map(|b| RblBeat {
            time_ms: b.time_ms,
            number: b.beat_number,
            tempo_x100: b.tempo_x100,
        })
        .collect();
    let owned_key = analysis.key.and_then(|k| CString::new(k.name).ok());
    let owned_columns: Vec<_> = analysis
        .waveform
        .columns
        .iter()
        .map(|c| RblColumn {
            low: c.low,
            mid: c.mid,
            high: c.high,
            peak: c.peak,
        })
        .collect();
    let owned_overview: Vec<_> = analysis
        .waveform
        .overview
        .iter()
        .map(|c| RblColumn {
            low: c[0],
            mid: c[1],
            high: c[2],
            peak: c.iter().copied().max().unwrap_or(0),
        })
        .collect();
    let mut result = RblResult {
        bpm: analysis.tempo.bpm,
        sample_rate: rate,
        duration_secs,
        beats: std::ptr::null(),
        beats_len: owned_beats.len(),
        key: std::ptr::null(),
        columns: std::ptr::null(),
        columns_len: owned_columns.len(),
        overview: std::ptr::null(),
        overview_len: owned_overview.len(),
        owned_beats,
        owned_key,
        owned_columns,
        owned_overview,
    };
    result.beats = result.owned_beats.as_ptr();
    result.key = result
        .owned_key
        .as_ref()
        .map_or(std::ptr::null(), |k| k.as_ptr());
    result.columns = result.owned_columns.as_ptr();
    result.overview = result.owned_overview.as_ptr();
    Some(result)
}

#[no_mangle]
pub extern "C" fn rbl_bridge_version() -> u32 {
    1
}

#[no_mangle]
/// AI-generated safety documentation.
/// # Safety
/// `path` must be null or a valid NUL-terminated UTF-8 byte string.
/// End of AI-generated safety documentation.
pub unsafe extern "C" fn rbl_analyze_path(path: *const c_char) -> *mut RblResult {
    if path.is_null() {
        return std::ptr::null_mut();
    }
    std::panic::catch_unwind(|| {
        let path = CStr::from_ptr(path).to_str().ok()?;
        analyze(Path::new(path)).map(|result| Box::into_raw(Box::new(result)))
    })
    .ok()
    .flatten()
    .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
/// AI-generated safety documentation.
/// # Safety
/// `result` must be null or a pointer returned by `rbl_analyze_path`, freed once.
/// End of AI-generated safety documentation.
pub unsafe extern "C" fn rbl_result_free(result: *mut RblResult) {
    if !result.is_null() {
        drop(Box::from_raw(result));
    }
}
