#include "bazel_helpers.h"

// std
#include <memory>

// Qt
#include <QDir>
#include <QProcess>
#include <QStringList>


namespace BazelProjectManager::Internal {

namespace {
std::string_view regexMatchToStringView(const std::ssub_match& match) {
  return std::string_view{
    match.first.base(),
    static_cast<std::string_view::size_type>(match.length())
  };
}

QByteArrayView regexMatchToByteArrayView(const std::ssub_match& match) {
  return QByteArrayView{
    match.first.base(),
    static_cast<qsizetype>(match.length())
  };
}

QString targetKindToQueryStr(QueryTargetKind kind) {
  switch (kind) {
    case QueryTargetKind::Rule: {
      return "rule";
    }
    case QueryTargetKind::SourceFile: {
      return "source file";
    }
    case QueryTargetKind::GeneratedFile: {
      return "generated file";
    }
    case QueryTargetKind::PackageGroup: {
      return "package group";
    }
    case QueryTargetKind::EnvGroup: {
      return "environment group";
    }
    default: {
      throw std::runtime_error{
          "Unsupported QueryTargetKind value: " +
          std::to_string(static_cast<std::underlying_type_t<QueryTargetKind>>(kind))
      };
    }
  }
}

}  // namespace

// --- BazelLabel ---

std::optional<BazelLabel> BazelLabel::parse(const std::string& label) {
  // TODO: Verify against actual Starlark syntax rules.
  static const std::regex labelRe{
    "^(@\\S+)?"           // repository
    "/((/[^/:]*)*)"       // package identifier
    ":"                   // target separator
    "(([^/:]+/)*[^/:]+)$" // target
  };
  std::smatch matchResults;
  if (!std::regex_match(label, matchResults, labelRe)) {
    return std::nullopt;
  }
  return BazelLabel{std::move(matchResults)};
}

BazelLabel::BazelLabel(std::smatch matchResults)
  : matchResults_{std::move(matchResults)}
{}

std::string_view BazelLabel::repo() const {
  return regexMatchToStringView(matchResults_[1]);
}

QByteArrayView BazelLabel::repoBA() const {
  return regexMatchToByteArrayView(matchResults_[1]);
}

std::string_view BazelLabel::packageDirPath() const {
  return regexMatchToStringView(matchResults_[2]);
}

QByteArrayView BazelLabel::packageDirPathBA() const {
  return regexMatchToByteArrayView(matchResults_[2]);
}

std::string_view BazelLabel::targetParentDirName() const {
  return regexMatchToStringView(matchResults_[3]);
}

QByteArrayView BazelLabel::targetParentDirNameBA() const {
  return regexMatchToByteArrayView(matchResults_[3]);
}

std::string_view BazelLabel::targetName() const {
  return regexMatchToStringView(matchResults_[4]);
}

QByteArrayView BazelLabel::targetNameBA() const {
  return regexMatchToByteArrayView(matchResults_[4]);
}

// --- RuleAttributeRefs ---

RuleAttributeRefs::RuleAttributeRefs(const blaze_query::Rule& rule) {
  for (int i = 0; i < rule.attribute_size(); i++) {
    const blaze_query::Attribute& attr = rule.attribute(i);

    if (attr.name() == "$is_executable") {
      is_executable = attr.has_boolean_value() && attr.boolean_value();
      continue;
    }
  }  // for
}

// ---

blaze_query::QueryResult bazelQuery(const QString& workspaceDir, const QString& query) {
  QProcess bazelProc;
  bazelProc.setWorkingDirectory(workspaceDir);
  bazelProc.start(
    "bazel",
    {
      "query", query,
      "--keep_going",  // Don't abort on errors.
      "--relative_locations",
      "--noimplicit_deps",  // Don't care about toolchains and such. Yet.
      "--output", "proto",
      "--order_output", "deps",  // Default for `proto` output, yet make it  explicit.
      // "--proto:output_rule_attrs", "$is_executable",
      // TODO: Set --proto:output_rule_attrs=... to attributes of actual interest.
    }
  );
  bazelProc.waitForFinished();

  // TODO: Use some streaming instead of storing the entire output in memory.
  const auto& bazelOutput = bazelProc.readAllStandardOutput();
  blaze_query::QueryResult queryResult;
  if (!queryResult.ParseFromArray(bazelOutput.data(), bazelOutput.size())) {
    throw std::runtime_error("Could not parse bazel output.");
  }
  return queryResult;
}


blaze_query::QueryResult queryPackage(
    const QString& workspaceDir, const QString& packageDirPath, const QueryTargetKind targetKinds
) {
  using U = std::underlying_type_t<QueryTargetKind>;
  using BT = ::blaze_query::Target;

  QStringList targetKindExprs;
  for (int bit = BT::Discriminator_MIN; bit <= BT::Discriminator_MAX; bit++) {
    const auto targetKind = static_cast<QueryTargetKind>(1 << bit);
    if (static_cast<U>(targetKind) & static_cast<U>(targetKinds)) {
      targetKindExprs.append(targetKindToQueryStr(targetKind));
    }
  }
  const auto kindsExpr =
      (targetKinds == QueryTargetKind::AllKinds) ? "*" : targetKindExprs.join("|");

  return bazelQuery(workspaceDir, QString{"kind(\"(%1)\", //%2:*)"}.arg(kindsExpr, packageDirPath));
}

QStringView compileModeToCLIArg(BazelCompilationMode mode) {
  static const QStringView fastbuild = u"fastbuild";
  static const QStringView dbg = u"dbg";
  static const QStringView opt = u"opt";

  switch (mode) {
    case BazelCompilationMode::Fast: return fastbuild;
    case BazelCompilationMode::Dbg: return dbg;
    case BazelCompilationMode::Opt: return opt;
    case BazelCompilationMode::CompileMode_LAST: break;
  }
  return fastbuild;
}

}  // namespace BazelProjectManager::Internal
