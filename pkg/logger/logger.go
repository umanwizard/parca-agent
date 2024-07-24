// Copyright 2022-2024 The Parca Authors
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package logger

import (
	"os"

	libbpf "github.com/aquasecurity/libbpfgo"
	"github.com/go-kit/log"
	"github.com/go-kit/log/level"
)

const (
	LogFormatLogfmt = "logfmt"
	LogFormatJSON   = "json"
)

// NewLogger returns a log.Logger that prints in the provided format at the
// provided level with a UTC timestamp and the caller of the log entry. If non
// empty, the debug name is also appended as a field to all log lines. Panics
// if the log level is not error, warn, info or debug. Log level is expected to
// be validated before passed to this function.
func NewLogger(logLevel, logFormat, debugName string) log.Logger {
	var (
		logger log.Logger
		lvl    level.Option
	)

	switch logLevel {
	case "error":
		lvl = level.AllowError()
	case "warn":
		lvl = level.AllowWarn()
	case "info":
		lvl = level.AllowInfo()
	case "debug":
		lvl = level.AllowDebug()
	default:
		// This enum is already checked and enforced by flag validations, so
		// this should never happen.
		panic("unexpected log level")
	}

	if logFormat == "" {
		logFormat = LogFormatLogfmt
	}

	logger = log.NewLogfmtLogger(log.NewSyncWriter(os.Stderr))

	if logFormat == LogFormatJSON {
		logger = log.NewJSONLogger(log.NewSyncWriter(os.Stderr))
	}

	logger = level.NewFilter(logger, lvl)

	if debugName != "" {
		logger = log.With(logger, "name", debugName)
	}

	return log.With(logger, "ts", log.DefaultTimestampUTC, "caller", log.DefaultCaller)
}

func NewLibbpfLogCallbacks(logger log.Logger) libbpf.Callbacks {
	return libbpf.Callbacks{
		Log: func(lvl int, msg string) {
			logger := log.With(logger, "component", "libbpf")
			switch lvl {
			case libbpf.LibbpfDebugLevel:
				logger = level.Debug(logger)
			case libbpf.LibbpfInfoLevel:
				logger = level.Info(logger)
			case libbpf.LibbpfWarnLevel:
				logger = level.Warn(logger)
			default:
			}
			// This is necessary to see long lines, helpful for qemu/go test bpf verifier error debugging.
			// os.Stderr.WriteString(msg)
			logger.Log("msg", msg)
		},
	}
}
