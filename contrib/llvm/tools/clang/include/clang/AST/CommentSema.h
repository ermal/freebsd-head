//===--- CommentSema.h - Doxygen comment semantic analysis ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the semantic analysis class for Doxygen comments.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_COMMENT_SEMA_H
#define LLVM_CLANG_AST_COMMENT_SEMA_H

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/AST/Comment.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Allocator.h"

namespace clang {
class Decl;
class SourceMgr;

namespace comments {
class CommandTraits;

class Sema {
  Sema(const Sema&);           // DO NOT IMPLEMENT
  void operator=(const Sema&); // DO NOT IMPLEMENT

  /// Allocator for AST nodes.
  llvm::BumpPtrAllocator &Allocator;

  /// Source manager for the comment being parsed.
  const SourceManager &SourceMgr;

  DiagnosticsEngine &Diags;

  const CommandTraits &Traits;

  /// Information about the declaration this comment is attached to.
  DeclInfo *ThisDeclInfo;

  /// Comment AST nodes that correspond to \c ParamVars for which we have
  /// found a \\param command or NULL if no documentation was found so far.
  ///
  /// Has correct size and contains valid values if \c DeclInfo->IsFilled is
  /// true.
  llvm::SmallVector<ParamCommandComment *, 8> ParamVarDocs;

  /// Comment AST nodes that correspond to parameter names in
  /// \c TemplateParameters.
  ///
  /// Contains a valid value if \c DeclInfo->IsFilled is true.
  llvm::StringMap<TParamCommandComment *> TemplateParameterDocs;

  /// AST node for the \\brief command and its aliases.
  const BlockCommandComment *BriefCommand;

  /// AST node for the \\returns command and its aliases.
  const BlockCommandComment *ReturnsCommand;

  DiagnosticBuilder Diag(SourceLocation Loc, unsigned DiagID) {
    return Diags.Report(Loc, DiagID);
  }

  /// A stack of HTML tags that are currently open (not matched with closing
  /// tags).
  SmallVector<HTMLStartTagComment *, 8> HTMLOpenTags;

public:
  Sema(llvm::BumpPtrAllocator &Allocator, const SourceManager &SourceMgr,
       DiagnosticsEngine &Diags, const CommandTraits &Traits);

  void setDecl(const Decl *D);

  /// Returns a copy of array, owned by Sema's allocator.
  template<typename T>
  ArrayRef<T> copyArray(ArrayRef<T> Source) {
    size_t Size = Source.size();
    if (Size != 0) {
      T *Mem = Allocator.Allocate<T>(Size);
      std::uninitialized_copy(Source.begin(), Source.end(), Mem);
      return llvm::makeArrayRef(Mem, Size);
    } else
      return llvm::makeArrayRef(static_cast<T *>(NULL), 0);
  }

  ParagraphComment *actOnParagraphComment(
      ArrayRef<InlineContentComment *> Content);

  BlockCommandComment *actOnBlockCommandStart(SourceLocation LocBegin,
                                              SourceLocation LocEnd,
                                              StringRef Name);

  void actOnBlockCommandArgs(BlockCommandComment *Command,
                             ArrayRef<BlockCommandComment::Argument> Args);

  void actOnBlockCommandFinish(BlockCommandComment *Command,
                               ParagraphComment *Paragraph);

  ParamCommandComment *actOnParamCommandStart(SourceLocation LocBegin,
                                              SourceLocation LocEnd,
                                              StringRef Name);

  void actOnParamCommandDirectionArg(ParamCommandComment *Command,
                                     SourceLocation ArgLocBegin,
                                     SourceLocation ArgLocEnd,
                                     StringRef Arg);

  void actOnParamCommandParamNameArg(ParamCommandComment *Command,
                                     SourceLocation ArgLocBegin,
                                     SourceLocation ArgLocEnd,
                                     StringRef Arg);

  void actOnParamCommandFinish(ParamCommandComment *Command,
                               ParagraphComment *Paragraph);

  TParamCommandComment *actOnTParamCommandStart(SourceLocation LocBegin,
                                                SourceLocation LocEnd,
                                                StringRef Name);

  void actOnTParamCommandParamNameArg(TParamCommandComment *Command,
                                      SourceLocation ArgLocBegin,
                                      SourceLocation ArgLocEnd,
                                      StringRef Arg);

  void actOnTParamCommandFinish(TParamCommandComment *Command,
                                ParagraphComment *Paragraph);

  InlineCommandComment *actOnInlineCommand(SourceLocation CommandLocBegin,
                                           SourceLocation CommandLocEnd,
                                           StringRef CommandName);

  InlineCommandComment *actOnInlineCommand(SourceLocation CommandLocBegin,
                                           SourceLocation CommandLocEnd,
                                           StringRef CommandName,
                                           SourceLocation ArgLocBegin,
                                           SourceLocation ArgLocEnd,
                                           StringRef Arg);

  InlineContentComment *actOnUnknownCommand(SourceLocation LocBegin,
                                            SourceLocation LocEnd,
                                            StringRef Name);

  TextComment *actOnText(SourceLocation LocBegin,
                         SourceLocation LocEnd,
                         StringRef Text);

  VerbatimBlockComment *actOnVerbatimBlockStart(SourceLocation Loc,
                                                StringRef Name);

  VerbatimBlockLineComment *actOnVerbatimBlockLine(SourceLocation Loc,
                                                   StringRef Text);

  void actOnVerbatimBlockFinish(VerbatimBlockComment *Block,
                                SourceLocation CloseNameLocBegin,
                                StringRef CloseName,
                                ArrayRef<VerbatimBlockLineComment *> Lines);

  VerbatimLineComment *actOnVerbatimLine(SourceLocation LocBegin,
                                         StringRef Name,
                                         SourceLocation TextBegin,
                                         StringRef Text);

  HTMLStartTagComment *actOnHTMLStartTagStart(SourceLocation LocBegin,
                                              StringRef TagName);

  void actOnHTMLStartTagFinish(HTMLStartTagComment *Tag,
                               ArrayRef<HTMLStartTagComment::Attribute> Attrs,
                               SourceLocation GreaterLoc,
                               bool IsSelfClosing);

  HTMLEndTagComment *actOnHTMLEndTag(SourceLocation LocBegin,
                                     SourceLocation LocEnd,
                                     StringRef TagName);

  FullComment *actOnFullComment(ArrayRef<BlockContentComment *> Blocks);

  void checkBlockCommandEmptyParagraph(BlockCommandComment *Command);

  void checkReturnsCommand(const BlockCommandComment *Command);

  /// Emit diagnostics about duplicate block commands that should be
  /// used only once per comment, e.g., \\brief and \\returns.
  void checkBlockCommandDuplicate(const BlockCommandComment *Command);

  bool isFunctionDecl();
  bool isTemplateOrSpecialization();

  ArrayRef<const ParmVarDecl *> getParamVars();

  /// Extract all important semantic information from
  /// \c ThisDeclInfo->ThisDecl into \c ThisDeclInfo members.
  void inspectThisDecl();

  /// Returns index of a function parameter with a given name.
  unsigned resolveParmVarReference(StringRef Name,
                                   ArrayRef<const ParmVarDecl *> ParamVars);

  /// Returns index of a function parameter with the name closest to a given
  /// typo.
  unsigned correctTypoInParmVarReference(StringRef Typo,
                                         ArrayRef<const ParmVarDecl *> ParamVars);

  bool resolveTParamReference(StringRef Name,
                              const TemplateParameterList *TemplateParameters,
                              SmallVectorImpl<unsigned> *Position);

  StringRef correctTypoInTParamReference(
                              StringRef Typo,
                              const TemplateParameterList *TemplateParameters);

  InlineCommandComment::RenderKind
  getInlineCommandRenderKind(StringRef Name) const;

  bool isHTMLEndTagOptional(StringRef Name);
  bool isHTMLEndTagForbidden(StringRef Name);
};

} // end namespace comments
} // end namespace clang

#endif

